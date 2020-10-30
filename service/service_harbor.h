#ifndef TINK_SERVICE_HARBOR_H
#define TINK_SERVICE_HARBOR_H

#include <base_module.h>
#include <string_view>

namespace tink::Service {
    class ServiceHarbor : public BaseModule {
    public:
        ServiceHarbor();
        int Init(ContextPtr ctx, std::string_view param) override;

    private:
        static int CallBack_(Context& ctx, void* ud, int type, int session, uint32_t source, DataPtr& msg, size_t sz);
        ContextPtr ctx_;
        int id_;
        uint32_t slave_;

    };
}




#endif //TINK_SERVICE_HARBOR_H
