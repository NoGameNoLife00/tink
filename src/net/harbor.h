
#ifndef TINK_HARBOR_H
#define TINK_HARBOR_H
#include <common.h>
#include <context.h>

#define HarborInstance tink::Singleton<tink::Harbor>::GetInstance()

namespace tink {
    class Harbor {
    public:
        void Init(int harbor);
        void Start(ContextPtr ctx);
        void Exit();
        void Send(RemoteMessagePtr r_msg, uint32_t source, int session);
        int MessageIsRemote(uint32_t handle);

    private:
        static uint32_t harbor_;
        static ContextPtr remote_;
    };
}




#endif //TINK_HARBOR_H
