#include "context.h"
#include "context_manage.h"
#include <module_manage.h>
#include <error_code.h>
#include <spdlog/spdlog.h>
#include <common.h>
#include <timer.h>
#include <harbor.h>

namespace tink {
    std::atomic_int Context::total = 0;

    void Context::Destroy() {
        mod_->Release();
        queue_->MarkRelease();
    }

    void Context::Send(DataPtr &&data, size_t sz, uint32_t source, int type, int session) {
        TinkMessage msg;
        msg.source = source;
        msg.session = session;
        msg.data = std::move(data);
        msg.size = sz | (static_cast<size_t>(type) << MESSAGE_TYPE_SHIFT);
        queue_->Push(msg);
    }

    void Context::SetCallBack(const ContextCallBack &cb, void *ud) {
        this->callback_ = cb;
        cb_ud_ = ud;
    }

    int Context::NewSession() {
        int session = ++session_id_;
        if (session <= 0) {
            session_id_ = 1;
            return session_id_;
        }
        return session;
    }

    void Context::DispatchAll() {
        TinkMessage msg;
        while (queue_->Pop(msg, true)) {
            DispatchMessage(msg);
        }
    }

    void Context::DispatchMessage(TinkMessage &msg) {
        assert(init_);
        if (!callback_) {
            return;
        }
        std::lock_guard<Mutex> guard(mutex_);
        CurrentHandle::SetHandle(handle_);
        int type = msg.size >> MESSAGE_TYPE_SHIFT;
        size_t sz = msg.size & MESSAGE_TYPE_MASK;
        message_count_++;
        if (profile_) {
            cpu_start_ = TimeUtil::GetThreadTime();
            callback_(*this, cb_ud_, type, msg.session, msg.source, msg.data, sz);
            uint64_t cost_tm = TimeUtil::GetThreadTime() - cpu_start_;
            cpu_cost_ += cost_tm;
        } else {
            callback_(*this, cb_ud_, type, msg.session, msg.source, msg.data, sz);
        }
    }

    int Context::Send(uint32_t source, uint32_t destination, int type, int session, DataPtr &data,
                      size_t sz) {
        if ((sz & MESSAGE_TYPE_MASK) != sz) {
            spdlog::error("The message to {} is too large", destination);
            return E_PACKET_SIZE;
        }
        FilterArgs_(type, session, data, sz);
        if (source == 0) {
            source = handle_;
        }
        if (destination == 0) {
            if (data) {
                spdlog::error("Destination address can't be 0");
                data.reset();
                return E_FAILED;
            }
            return session;
        }
        if (HarborInstance.MessageIsRemote(destination)) {
            RemoteMessagePtr r_msg = std::make_shared<RemoteMessage>();
            r_msg->destination.handle = destination;
            r_msg->message = data;
            r_msg->size = sz & MESSAGE_TYPE_MASK;
            r_msg->type = sz >> MESSAGE_TYPE_SHIFT;
            HarborInstance.Send(r_msg, source, session);
        } else {
            TinkMessage s_msg;
            s_msg.source = source;
            s_msg.session = session;
            s_msg.data = data;
            s_msg.size = sz;
            if (CONTEXT_MNG.PushMessage(destination, s_msg)) {
                return E_FAILED;
            }
        }
        return session;
    }

    int Context::FilterArgs_(int type, int &session, DataPtr &data, size_t &sz) {
//        int needcopy = !(type & PTYPE_TAG_DONTCOPY);
        int allocsession = type & PTYPE_TAG_ALLOCSESSION;
        type &= 0xff;

        if (allocsession) {
            assert(session == 0);
            session = NewSession();
        }

        sz |= static_cast<size_t>(type) << MESSAGE_TYPE_SHIFT;
        return 0;
    }

    static void CopyName(char name[GLOBALNAME_LENGTH], const char * addr) {
        int i;
        for (i=0; i < GLOBALNAME_LENGTH && addr[i]; i++) {
            name[i] = addr[i];
        }
        for (; i < GLOBALNAME_LENGTH; i++) {
            name[i] = '\0';
        }
    }

    int Context::SendName(uint32_t source, const std::string &addr, int type, int session, DataPtr &data, size_t sz) {
        if (source == 0) {
            source = handle_;
        }
        uint32_t  des = 0;
        if (addr[0] == ':') {
            des = strtoul(addr.c_str()+1, NULL, 16);
        } else if ( addr[0] == '.') {
            des = CONTEXT_MNG.FindName(addr.c_str() + 1);
            if (des == 0) {
                return E_FAILED;
            }
        } else {
            if ((sz & MESSAGE_TYPE_MASK) != sz) {
                spdlog::error("The message to {} is too large", addr);
                return E_FAILED;
            }
            FilterArgs_(type, session, data, sz);

            RemoteMessagePtr r_msg = std::make_shared<RemoteMessage>();
            CopyName(r_msg->destination.name, addr.c_str());
            r_msg->destination.handle = 0;
            r_msg->message = data;
            r_msg->size = sz & MESSAGE_TYPE_MASK;
            r_msg->type = sz >> MESSAGE_TYPE_SHIFT;
            HarborInstance.Send(r_msg, source, session);
            return session;
        }
        return Send(source, des, type, session, data, sz);
    }

    void Context::DropMessage(TinkMessage &msg, void *ud) {
        struct DropT *d = static_cast<DropT *>(ud);
        msg.data.reset();
        uint32_t source = d->handle;
        assert(source);
    }
}

