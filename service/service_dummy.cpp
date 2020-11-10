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

}
