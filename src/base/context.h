#ifndef TINK_CONTEXT_H
#define TINK_CONTEXT_H

#include <base_module.h>
#include <type.h>

namespace tink {
    class Context;
    typedef std::shared_ptr<Context> ContextPtr;
    typedef std::function<int (Context& ctx, void* ud, int type, int session, uint32_t source, const void* msg, size_t sz)> ContextCallBack;
    class Context {
    public:
        int Init(const std::string& name, const std::string& param);
        void Destory();
        ContextCallBack cb;
        void *cb_ud;
        ModulePtr mod;
        uint64_t cpu_cost;
        uint64_t cpu_start;
        int session_id;
        int ref;
        int message_count;
        bool init;
        bool endless;
        bool profile;
        uint32_t handle;
    };
}



#endif //TINK_CONTEXT_H
