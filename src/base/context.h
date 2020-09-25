#ifndef TINK_CONTEXT_H
#define TINK_CONTEXT_H

#include <base_module.h>
#include <common.h>
#include <atomic>
#include "global_mq.h"

namespace tink {
    class Context;
    typedef std::shared_ptr<Context> ContextPtr;
    typedef std::function<int (Context& ctx, void* ud, int type, int session, uint32_t source, DataPtr& msg, size_t sz)> ContextCallBack;
    class Context : public std::enable_shared_from_this<Context> {
    public:
        static int Total() { return total.load(); }
        int Init(const std::string& name, const char * param);
        void Destroy();
        void Send(DataPtr &data, size_t sz, uint32_t source, int type, int session);
        void SetCallBack(ContextCallBack cb, void *ud);
        uint32_t Handle() { return handle_; }
        void Reserve() { --total; }
        int NewSession();
        MQPtr Queue() { return queue_; }
        bool Endless() { return endless_; }
        void SetEndless(bool b) { endless_ = b; }
        void DispatchAll();
        void *cb_ud;
        int Send(uint32_t source, uint32_t destination, int type, int session, DataPtr &data, size_t sz);
        int SendName(uint32_t source, const std::string& addr, int type, int session, DataPtr &data, size_t sz);
        ModulePtr mod;

        uint64_t cpu_cost;
        uint64_t cpu_start;
        int session_id;
        int ref;
        bool init;
        bool profile;

    private:
        int FilterArgs_(int type, int &session, DataPtr& data, size_t &sz);
        void DispatchMessage_(MsgPtr msg);

        mutable std::mutex mutex_;
        static std::atomic_int total;
        static int handle_key;
        MQPtr queue_;
        bool endless_;
        uint32_t handle_;
        int message_count_;
        ContextCallBack callback_;
    };
}



#endif //TINK_CONTEXT_H
