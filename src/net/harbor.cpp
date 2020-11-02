#include <harbor.h>

namespace tink {
    static inline int invalid_type(int type) {
        return type != PTYPE_SYSTEM && type != PTYPE_HARBOR;
    }

    void Harbor::Send(const tink::RemoteMessagePtr& r_msg, uint32_t source, int session) {
        assert(invalid_type(r_msg->type) && remote_);
        remote_->Send(std::static_pointer_cast<void>(r_msg), sizeof(*r_msg), source, PTYPE_SYSTEM, session);
    }

    void Harbor::Init(int harbor) {
        harbor_ = static_cast<uint32_t>(harbor) << HANDLE_REMOTE_SHIFT;
    }

    void Harbor::Start(ContextPtr ctx) {
        ctx->Reserve();
        remote_ = ctx;
    }

    void Harbor::Exit() {
        remote_.reset();
    }

    int Harbor::MessageIsRemote(uint32_t handle) const {
        assert(harbor_ != ~0);
        int h = (handle & ~HANDLE_MASK);
        return h != harbor_ && h != 0;
    }
}

