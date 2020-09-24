#include "context.h"
#include "handle_manage.h"
#include <module_manage.h>
#include <error_code.h>
#include <spdlog/spdlog.h>
#include <common.h>
#include <timer.h>
namespace tink {
    std::atomic_int Context::total = 0;
    struct drop_t {
        uint32_t handle;
    };
    static void DropMessage(MsgPtr msg, void *ud) {
        struct drop_t *d = static_cast<drop_t *>(ud);
        assert(d->handle);
        // todo send error to msg source

    }

    int Context::Init(const std::string &name, const char *param) {
        auto mod = ModuleMngInstance.Query(name);
        if (!mod) {
            return E_QUERY_MODULE;
        }
        this->mod = mod;
        callback_ = nullptr;
        cb_ud = nullptr;
        session_id = 0;
        init = false;
        endless_ = false;
        cpu_cost = 0;
        cpu_start = 0;
        profile = false;
        message_count_ = 0;
        handle_ = HandleMngInstance.Register(shared_from_this());
        if (handle_ == 0) {
            return E_FAILED;
        }
        queue_ = std::make_shared<MessageQueue>(handle_);
        mutex_.lock();
        int ret = mod->Init(*this, param);
        mutex_.unlock();
        if (ret == E_OK) {
            init = true;
            GlobalMQInstance.Push(queue_);
        } else {
            spdlog::error("Failed launch {}", name);
            HandleMngInstance.Unregister(handle_);
            struct drop_t d = {handle_};
            queue_->Release(DropMessage, &d);
        }
        ++total;
        return ret;
    }

    void Context::Destroy() {
        mod->Release();
        queue_->MarkRelease();
        --total;
    }

    void Context::Send(DataPtr &data, size_t sz, uint32_t source, int type, int session) {
        MsgPtr msg = std::make_shared<Message>();
        msg->source = source;
        msg->session = session;
        msg->data = std::move(data);
        msg->size = sz | (static_cast<size_t>(type) << MESSAGE_TYPE_SHIFT);
        queue_->Push(msg);
    }

    void Context::SetCallBack(ContextCallBack cb, void *ud) {
        this->callback_ = cb;
        cb_ud = ud;
    }

    int Context::NewSession() {
        int session = ++session_id;
        if (session <= 0) {
            session_id = 1;
            return session_id;
        }
        return session;
    }

    void Context::DispatchAll() {
        MsgPtr msg = queue_->Pop();;
        while (msg) {
            DispatchMessage_(msg);
            msg = queue_->Pop();
        }
    }

    void Context::DispatchMessage_(MsgPtr msg) {
        assert(init);
        std::lock_guard<Mutex> guard(mutex_);
        pthread_setspecific(HandleMngInstance.handle_key, (void *)(uintptr_t)(handle_));
        int type = msg->size >> MESSAGE_TYPE_SHIFT;
        size_t sz = msg->size & MESSAGE_TYPE_MASK;
        message_count_++;
        if (profile) {
            cpu_start = TimeUtil::GetThreadTime();
            callback_(*this, cb_ud, type, msg->session, msg->source, msg->data, sz);
            uint64_t cost_tm = TimeUtil::GetThreadTime() - cpu_start;
            cpu_cost += cost_tm;
        } else {
            callback_(*this, cb_ud, type, msg->session, msg->source, msg->data, sz);
        }
    }

    int Context::Send(uint32_t source, uint32_t destination, int type, int session, DataPtr &data,
                      size_t sz) {
        if ((sz & MESSAGE_TYPE_MASK) != sz) {
            spdlog::error("The message to {} is too large", destination);
            return E_PACKET_SIZE;
        }
        FilterArgs_(type, &session, data, &sz);
        if (source == 0) {
            source = handle_;
        }
        if (destination == 0) {
            if (data) {
                spdlog::error("Destination address can't be 0");
                data.reset();
                return E_FAILED;
            }
        }


    }

    int Context::FilterArgs_(int type, int *session, DataPtr &data, size_t *sz) {
//        int needcopy = !(type & PTYPE_TAG_DONTCOPY);
        int allocsession = type & PTYPE_TAG_ALLOCSESSION;
        type &= 0xff;

        if (allocsession) {
            assert(*session == 0);
            *session = NewSession();
        }

        *sz |= (size_t)type << MESSAGE_TYPE_SHIFT;
        return 0;
    }

}

