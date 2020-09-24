
#ifndef TINK_HARBOR_H
#define TINK_HARBOR_H
#include <common.h>
#include <context.h>

namespace tink {
    class Harbor {
    public:
        static void Init(int harbor);
        static void Start(ContextPtr ctx);
        static void Exit();
        static void Send(RemoteMsgPtr r_msg, uint32_t source, int session);
        static int MessageIsRemote(uint32_t handle);

    private:
        static uint32_t HARBOR;
        static ContextPtr REMOTE;
    };
}




#endif //TINK_HARBOR_H
