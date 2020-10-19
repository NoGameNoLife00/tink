#ifndef TINK_SERVICE_GATE_H
#define TINK_SERVICE_GATE_H
#include <base_module.h>
#include <data_buffer.h>
#include <memory>
#include <common.h>
#include <string>

namespace tink::Service {
    typedef struct Connection_ {
        int id;	// socket id
        uint32_t agent;
        uint32_t client;
        char remote_name[32];
        DataBuffer buffer;
    }Connection;
//    typedef std::shared_ptr<Connection> ConnectionPtr;

    class ServiceGate : public BaseModule {
    public:
        ServiceGate();

        int Init(ContextPtr ctx, const std::string &param) override;
        int StartListen(std::string& listen_addr);
        void Ctrl(DataPtr &msg, int sz);
        void Release() override;
        void DispatchSocketMessage(TinkSocketMsgPtr msg, int sz);
        void DispatchMessage(Connection &c, int id, DataPtr data, int sz);
        typedef PoolSet<Connection> ConnPool;
        ContextPtr ctx;
        int listen_id;
        uint32_t watchdog;
        uint32_t broker;
        int client_tag;
        int header_size;
        int max_connection;
        std::unordered_map<int, Connection*> conn;
        std::shared_ptr<ConnPool> conn_pool;
        std::shared_ptr<MessagePool> msg_pool;
    private:
        static int CallBack_(Context& ctx, void* ud, int type, int session, uint32_t source, DataPtr& msg, size_t sz);
        void ForwardAgent_(int fd, uint32_t agent_addr, uint32_t client_addr);
        void Forward_(Connection& c, int size);
        void Report_(const char * data, ...);
    };
}



#endif //TINK_SERVICE_GATE_H
