#include <string_view>
#include <error_code.h>
#include <sstream>
#include <harbor.h>
#include "service_harbor.h"

namespace tink::Service {
    ServiceHarbor::ServiceHarbor() {

    }

    int ServiceHarbor::Init(ContextPtr ctx, std::string_view param) {
        if (param.empty()) {
            return E_FAILED;
        }
        ctx_ = ctx;

        InitLog();
        std::istringstream is(param.data());
        is >> id_ >> slave_;
        if (slave_ == 0) {
            return E_FAILED;
        }
        ctx_->SetCallBack(CallBack_, this);
        HARBOR.Start(ctx_);
        return 0;
    }

    int
    ServiceHarbor::CallBack_(Context &ctx, void *ud, int type, int session, uint32_t source, DataPtr &msg, size_t sz) {
        return 0;
    }

}
