#include <harbor.h>
#include <error_code.h>
#include "service_dummy.h"

namespace tink::Service {
    int ServiceDummy::Init(std::shared_ptr<Context> ctx, std::string_view param) {
        ctx_ = ctx;
        HARBOR.Start(ctx);
        ctx->SetCallBack(MainLoop_, this);
    }

    void ServiceDummy::Release() {
        BaseModule::Release();
    }

    int ServiceDummy::MainLoop_(Context &ctx, void *ud, int type, int session, uint32_t source, DataPtr msg, size_t sz) {
        auto *d = static_cast<ServiceDummy*>(ud);
        switch (type) {
            case PTYPE_SYSTEM: {
                RemoteMessagePtr r_msg = std::reinterpret_pointer_cast<RemoteMessage>(msg);
                assert(sz == sizeof(r_msg->destination));

                return E_OK;
            }


        }
    }

    void ServiceDummy::UpdateName_(const std::string &name, uint32_t handle) {
        auto it = map_.find(name);
        if (it == map_.end()) {
            bool ret;
            std::tie(it, ret) = map_.emplace(name, std::pair(handle, nullptr));
        }
        std::pair<int, HarborMsgQueuePtr>& val = it->second;
        val.first = handle;
        if (val.second) {
            DispatchQueue_(it);
            val.second.reset();
        }
    }

    void ServiceDummy::SendName_(uint32_t source, const std::string &name, int type, int session, DataPtr msg, size_t sz) {

    }

    void ServiceDummy::DispatchQueue_(QueueMap::iterator &node) {
        QueueBind& val = node->second;
        MsgQueuePtr queue = val.second;
        uint32_t handle = val.first;
        auto & name = node->first;
        int harbor_id = handle >> HANDLE_REMOTE_SHIFT;
        if (queue) {
            for (Msg& m : *queue) {
                
            }
        }
    }

}
