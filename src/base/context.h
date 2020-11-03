#ifndef TINK_CONTEXT_H
#define TINK_CONTEXT_H

#include <base_module.h>
#include <common.h>
#include <atomic>
#include <global_mq.h>

namespace tink {
    class Context;
    class BaseModule;
    typedef std::shared_ptr<BaseModule> ModulePtr;
    typedef std::shared_ptr<Context> ContextPtr;
    typedef std::function<int (Context& ctx, void* ud, int type, int session, uint32_t source, DataPtr& msg, size_t sz)> ContextCallBack;

    struct DropT {
        uint32_t handle;
    };


    class Context : public std::enable_shared_from_this<Context> {
    public:
        Context() { ++total; }
        ~Context() {  }
        static int Total() { return total.load(); }
        static void DropMessage(TinkMessage &msg, void *ud);

        void Destroy();
        void Send(DataPtr data, size_t sz, uint32_t source, int type, int session);
        int Send(uint32_t source, uint32_t destination, int type, int session, DataPtr data, size_t sz);
        void SetCallBack(const ContextCallBack &cb, void *ud);
        uint32_t Handle() const { return handle_; }
        void Reserve() { --total; }
        int NewSession();
        MQPtr Queue() { return queue_; }
        bool Endless() const { return endless_; }
        void SetEndless(bool b) { endless_ = b; }
        void DispatchAll();
        int SendName(uint32_t source, std::string_view addr, int type, int session, DataPtr data, size_t sz);
        void DispatchMessage(TinkMessage &msg);
        ContextCallBack GetCallBack() {return callback_;}
        uint64_t GetCpuCost() const {return cpu_cost_;}
        bool GetProfile() const {return profile_;}
        uint64_t GetCpuStart() const {return cpu_start_;}
        int GetMessageCount() const {return message_count_;}

        void Exit(uint32_t handle);
        uint32_t ToHandle(std::string_view param);
        ModulePtr GetModule() {return mod_;}

        std::string Command(std::string_view cmd, std::string_view param);
        std::string result;
        friend class HandleStorage;
    private:
        static std::atomic_int total;

        int FilterArgs_(int type, int &session, DataPtr data, size_t &sz);

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
