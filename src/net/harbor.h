
#ifndef TINK_HARBOR_H
#define TINK_HARBOR_H
#include "common.h"
#include "base/context.h"



namespace tink {
    class Context;

    class Harbor {
    public:
        using ContextPtr = std::shared_ptr<Context>;

        void Init(int harbor);
        void Start(ContextPtr ctx);
        void Exit();
        void Send(RemoteMessagePtr r_msg, uint32_t source, int session);
        int MessageIsRemote(uint32_t handle) const;

    private:
        uint32_t harbor_;
        ContextPtr remote_;
    };
}




#endif //TINK_HARBOR_H
