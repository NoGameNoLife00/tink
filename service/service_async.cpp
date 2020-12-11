#include "service_async.h"

namespace tink::Service {

    int ServiceAsync::Init(ContextPtr ctx, std::string_view param) {
        ctx_ = ctx;
        BytePtr tmp(new byte[param.length()+1], std::default_delete<byte[]>());
        memcpy(tmp.get(), param.data(), param.length());
        tmp[param.length()] = '\0';

        ctx_->SetCallBack(LaunchCb_, this);
        std::string self = ctx_->Command("REG", "");
        uint32_t handle_id = strtol(self.data()+1, nullptr, 16);
        ctx_->Send(0, handle_id, PTYPE_TAG_DONTCOPY, 0, tmp, param.length()+1);
    }

    int
    ServiceAsync::LaunchCb_(void *ud, int type, int session, uint32_t source, DataPtr msg, size_t sz) {
        assert(type == 0 && session == 0);
        auto* service = static_cast<ServiceAsync *>(ud);
        service->ctx_->SetCallBack(nullptr, nullptr);

        return 0;
    }


}