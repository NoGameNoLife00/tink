#ifndef TINK_SERVICE_DUMMY_H
#define TINK_SERVICE_DUMMY_H

#include <base_module.h>

namespace tink::Service {
    class ServiceDummy : public BaseModule {
    public:
        int Init(ContextPtr ctx, std::string_view param) override;

        void Release() override;
    private:
        static int MainLoop_(Context& ctx, void* ud, int type, int session, uint32_t source, DataPtr msg, size_t sz);

        ContextPtr ctx_;
    };
}




#endif //TINK_SERVICE_DUMMY_H
