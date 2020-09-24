#include "harbor.h"

namespace tink {
    uint32_t Harbor::HARBOR = ~0;
    ContextPtr REMOTE = nullptr;
    static inline int invalid_type(int type) {
        return type != PTYPE_SYSTEM && type != PTYPE_HARBOR;
    }

    void Harbor::Send(tink::RemoteMsgPtr r_msg, uint32_t source, int session) {
        assert(invalid_type(r_msg->type) && REMOTE);
        REMOTE->Send(r_msg, sizeof(*r_msg), source, PTYPE_SYSTEM, session);
    }

    void Harbor::Init(int harbor) {
        HARBOR = static_cast<uint32_t>(harbor) << HANDLE_REMOTE_SHIFT;
    }

    void Harbor::Start(ContextPtr ctx) {
        ctx->Reserve();
        REMOTE = ctx;
    }

    void Harbor::Exit() {
        REMOTE.reset();
    }


}

