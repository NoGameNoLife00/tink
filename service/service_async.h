#ifndef TINK_SERVICE_ASYNC_H
#define TINK_SERVICE_ASYNC_H

#include "base/base_module.h"

namespace tink::Service {
    class ServiceAsync : public BaseModule {
    public:

        int Init(ContextPtr ctx, std::string_view param) override;

        void Release() override;

        void Signal(int signal) override;


    private:

        static int LaunchCb_(void *ud, int type, int session, uint32_t source, DataPtr msg, size_t sz);
        ContextPtr ctx_;
    };
}




#endif //TINK_SERVICE_ASYNC_H
