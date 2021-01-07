#ifndef TINK_SERVICE_GATE_H
#define TINK_SERVICE_GATE_H

#include <memory>
#include <common.h>
#include <string>
#include <string_view>
#include "base/context.h"
#include "net/socket.h"
#include "data_buffer.h"
#include "base/base_module.h"


namespace tink::Service {
    typedef struct Connection_ {
        int id;	// socket id
        uint32_t agent;
        uint32_t client;
        char remote_name[32];
        DataBuffer buffer;
    } Connection;
//    typedef std::shared_ptr<Connection> ConnectionPtr;

    class ServiceGate : public BaseModule {
    public:
        ServiceGate();
        int Init(ContextPtr ctx, std::string_view param) override;
        int StartListen(std::string_view listen_addr);
        void Ctrl(DataPtr &msg, int sz);
        void Release() override;
        void DispatchSocketMessage(TinkSocketMsgPtr msg, int sz);
        void DispatchMessage(Connection &c, int id, DataPtr data, int sz);
        typedef PoolSet<Connection> ConnPool;
    private:
        static int CallBack_(void *ud, int type, int session, uint32_t source, DataPtr msg, size_t sz);
        void ForwardAgent_(int fd, uint32_t agent_addr, uint32_t client_addr);
        void Forward_(Connection& c, int size);
        void Report_(const char * data, ...);
        ContextPtr ctx_;
        int listen_id_;
        uint32_t watchdog_;
        uint32_t broker_;
        int client_tag_;
        int header_size_;
        int max_connection_;
        std::unordered_map<int, Connection*> conn_;
        std::shared_ptr<ConnPool> conn_pool_;
        std::shared_ptr<MessagePool> msg_pool_;
    };

}




#endif //TINK_SERVICE_GATE_H
