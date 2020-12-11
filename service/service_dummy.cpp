#include <harbor.h>
#include <error_code.h>
#include "service_dummy.h"

namespace tink::Service {
    int ServiceDummy::Init(std::shared_ptr<Context> ctx, std::string_view param) {
        ctx_ = ctx;
        HARBOR.Start(ctx);
        ctx_->SetCallBack(this, 0, 0, 0, tink::DataPtr(), 0);
    }

    void ServiceDummy::Release() {
        BaseModule::Release();
    }

    int ServiceDummy::MainLoop_(void *ud, int type, int session, uint32_t source, DataPtr msg, size_t sz) {
        auto *d = static_cast<ServiceDummy*>(ud);
        RemoteMessagePtr r_msg = std::reinterpret_pointer_cast<RemoteMessage>(msg);
        switch (type) {
            case PTYPE_SYSTEM: {
                assert(sz == sizeof(r_msg->destination));
                d->UpdateName_(r_msg->destination.name, r_msg->destination.handle);
                return E_OK;
            }
            default: {
                if (r_msg->destination.handle == 0) {
                    d->SendName_(source, r_msg->destination.name, type, session, r_msg->message, r_msg->size);
                } else {
                    d->ctx_->Send(source, r_msg->destination.handle, type, session, r_msg->message, r_msg->size);
                }
                return 0;
            }
        }
    }

    void ServiceDummy::UpdateName_(const std::string &name, uint32_t handle) {
        auto it = map_.find(name);
        if (it == map_.end()) {
            bool ret;
            std::tie(it, ret) = map_.emplace(name, std::pair(handle, nullptr));
        }
        auto& val = it->second;
        val.first = handle;
        if (val.second) {
            DispatchQueue_(it);
            val.second.reset();
        }
    }

    void ServiceDummy::SendName_(uint32_t source, const std::string &name, int type, int session, DataPtr msg, size_t sz) {
        auto it = map_.find(name);
        if (it == map_.end()) {
            bool ret;
            std::tie(it, ret) = map_.emplace(name, std::pair(0, nullptr));
        }
        auto& val = it->second;
        if (val.first == 0) {
            if (val.second == nullptr) {
                val.second = std::make_shared<HarborMsgQueue>();
            }
            RemoteMsgHeader header;
            header.source = source;
            header.destination = type << HANDLE_REMOTE_SHIFT;
            header.session = session;
            PushQueue_(*val.second, msg, sz, header);
        } else {
            ctx_->Send(source, val.first, type, session, msg, sz);
        }
    }

    void ServiceDummy::DispatchQueue_(HarborMap::iterator &node) {
        auto& val = node->second;
        HarborMsgQueuePtr queue = val.second;
        uint32_t handle = val.first;
        auto & name = node->first;
        if (queue) {
            for (auto& m : *queue) {
                int type = m.header.destination >> HANDLE_REMOTE_SHIFT;
                ctx_->Send(m.header.source, handle, type, m.header.session, m.buffer, m.size);
            }
            queue->clear();
        }
    }

    void ServiceDummy::PushQueue_(HarborMsgQueue &queue, DataPtr buffer, size_t sz, RemoteMsgHeader &header) {
        HarborMsg m;
        m.header = header;
        m.buffer = std::move(buffer);
        m.size = sz;
        queue.emplace_back(m);
    }

}
