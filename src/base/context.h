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

    struct DropT {
        uint32_t handle;
    };
    class Context : public std::enable_shared_from_this<Context> {
    public:
        Context() {
            ++total;
        }
        ~Context() {
            --total;
        }
        static int Total() { return total.load(); }
        void Destroy();
        void Send(DataPtr &&data, size_t sz, uint32_t source, int type, int session);
        void SetCallBack(const ContextCallBack &cb, void *ud);
        uint32_t Handle() { return handle_; }
        void Reserve() { }
        int NewSession();
        MQPtr Queue() { return queue_; }
        bool Endless() { return endless_; }
        void SetEndless(bool b) { endless_ = b; }
        void DispatchAll();

        int Send(uint32_t source, uint32_t destination, int type, int session, DataPtr &data, size_t sz);
        int SendName(uint32_t source, const std::string& addr, int type, int session, DataPtr &data, size_t sz);

        static void DropMessage(TinkMessage &msg, void *ud);
        void DispatchMessage(TinkMessage &msg);
        ContextCallBack GetCallBack() {return callback_;}
        friend class ContextManage;
    private:
        static std::atomic_int total;
        static int handle_key;


        int FilterArgs_(int type, int &session, DataPtr& data, size_t &sz);

        mutable std::mutex mutex_;
        MQPtr queue_;
        bool endless_;
        uint32_t handle_;
        int message_count_;
        ContextCallBack callback_;
        uint64_t cpu_cost_;
        uint64_t cpu_start_;
        int session_id_;
        int ref_;
        bool init_;
        bool profile_;
        void *cb_ud_;
        ModulePtr mod_;
    };
}



#endif //TINK_CONTEXT_H
