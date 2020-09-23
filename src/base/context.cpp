#include "context.h"
#include "handle_manage.h"
#include <module_manage.h>
#include <error_code.h>
#include <spdlog/spdlog.h>
#include <common.h>

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
        cb = nullptr;
        cb_ud = nullptr;
        session_id = 0;
        init = false;
        endless = false;
        cpu_cost = 0;
        cpu_start = 0;
        profile = false;
        handle = HandleMngInstance.Register(shared_from_this());
        if (handle == 0) {
            return E_FAILED;
        }
        queue = std::make_shared<MessageQueue>(handle);
        mutex_.lock();
        int ret = mod->Init(*this, param);
        mutex_.unlock();
        if (ret == E_OK) {
            init = true;
            GlobalMQInstance.Push(queue);
        } else {
            spdlog::error("Failed launch {}", name);
            HandleMngInstance.Unregister(handle);
            struct drop_t d = {handle};
            queue->Release(DropMessage, &d);
        }
        ++total;
        return ret;
    }

    void Context::Destroy() {
        mod->Release();
        queue->MarkRelease();
        --total;
    }

    void Context::Send(BytePtr &data, size_t sz, uint32_t source, int type, int session) {
        MsgPtr msg = std::make_shared<Message>();
        msg->source = source;
        msg->session = session;
        msg->data = std::move(data);
        msg->size = sz | (static_cast<size_t>(type) << MESSAGE_TYPE_SHIFT);
        queue->Push(msg);
    }

    void Context::SetCallBack(ContextCallBack cb, void *ud) {
        this->cb = cb;
        cb_ud = ud;
    }

}

