#ifndef TINK_CONTEXT_H
#define TINK_CONTEXT_H

#include <base_module.h>
#include <common.h>
#include <atomic>
#include "global_mq.h"

namespace tink {
    class Context;
    typedef std::shared_ptr<Context> ContextPtr;
    typedef std::function<int (Context& ctx, void* ud, int type, int session, uint32_t source, const void* msg, size_t sz)> ContextCallBack;
    class Context : public std::enable_shared_from_this<Context> {
    public:
        static int Total() { return total.load(); }
        int Init(const std::string& name, const char * param);
        void Destroy();
        void Send(BytePtr &data, size_t sz, uint32_t source, int type, int session);
        void SetCallBack(ContextCallBack cb, void *ud);
        uint32_t Handle() { return handle; }

        ContextCallBack cb;
        void *cb_ud;
        ModulePtr mod;
        MQPtr queue;
        uint64_t cpu_cost;
        uint64_t cpu_start;
        int session_id;
        int ref;
        bool init;
        bool endless;
        bool profile;
        uint32_t handle;
    private:
        mutable std::mutex mutex_;
        static std::atomic_int total;
    };
}



#endif //TINK_CONTEXT_H
