#ifndef TINK_CONTEXT_H
#define TINK_CONTEXT_H

#include <atomic>
#include "common.h"
#include "base/base_module.h"
#include "base/global_mq.h"
#include "net/server.h"
#include "base/handle_manager.h"

namespace tink {
    class Context;
    class BaseModule;
    class Server;

    struct DropT {
        uint32_t handle;
    };

    class Context : public std::enable_shared_from_this<Context> {
    public:
        using ServerPtr = std::shared_ptr<Server>;
        using ModulePtr = std::shared_ptr<BaseModule>;
        using ContextCallBack = std::function<int (void* ud, int type, int session, uint32_t source, DataPtr msg, size_t sz)>;

        Context(ServerPtr s) : server_(s) { ++total; }
        ~Context() {  }
        static int Total() { return total.load(); }
        static void DropMessage(TinkMessage &msg, void *ud);

        ServerPtr GetServer() const { return server_; }
        void Destroy();
        void Send(DataPtr data, size_t sz, uint32_t source, int type, int session);
        int Send(uint32_t source, uint32_t destination, int type, int session, DataPtr data, size_t sz);
        void SetCallBack(const ContextCallBack &cb, void *ud);
        uint32_t Handle() const { return handle_; }
        void Reserve() { --total; }
        int NewSession();
        MsgQueuePtr Queue() { return queue_; }
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
        friend class HandleMgr;
    private:
        static std::atomic_int total;
        // Ԥ������Ϣ����
        int FilterArgs_(int type, int &session, DataPtr data, size_t &sz);
        ServerPtr server_;
        mutable std::mutex mutex_;
        MsgQueuePtr queue_; // ��Ϣ����
        bool endless_; // �Ƿ����
        uint32_t handle_; // Ψһid
        int message_count_; // �ۼƵ��յ�����Ϣ��
        ContextCallBack callback_; // ��Ϣ����Ļص�����, һ����module��init��������
        uint64_t cpu_cost_;
        uint64_t cpu_start_;
        int session_id_; // ���ͷ�����һ��session,�������ַ��ص���Ϣ
        bool init_; // �Ƿ��ʼ��
        bool profile_; // �Ƿ������ܼ��
        void *cb_ud_; // callback�ĵڶ��β���

        ModulePtr mod_; // module
    };

    using ContextPtr = std::shared_ptr<Context>;
}



#endif //TINK_CONTEXT_H
