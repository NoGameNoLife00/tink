#ifndef TINK_SERVICE_GATE_H
#define TINK_SERVICE_GATE_H
#include <base_module.h>
#include <data_buffer.h>
#include <memory>
#include <common.h>
namespace tink::Service {
    typedef struct Connection_ {
        int id;	// socket id
        uint32_t agent;
        uint32_t client;
        char remote_name[32];
        DataBuffer buffer;
    }Connection;
    typedef std::shared_ptr<Connection> ConnectionPtr;
    class ServiceGate : public BaseModule {
    public:
        ServiceGate();

        int Init(ContextPtr ctx, const std::string &param) override;
        static int CallBack(Context& ctx, void* ud, int type, int session, uint32_t source, DataPtr& msg, size_t sz);
        void Release() override;
        ContextPtr ctx;
        int listen_id;
        uint32_t watchdog;
        uint32_t broker;
        int client_tag;
        int header_size;
        int max_connection;
        std::vector<ConnectionPtr> conn;
        MessagePool mp;
    };
}



#endif //TINK_SERVICE_GATE_H
