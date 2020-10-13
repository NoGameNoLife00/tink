#ifndef TINK_SERVICE_GATE_H
#define TINK_SERVICE_GATE_H
#include <context.h>
namespace tink {
    struct connection {
        int id;	// socket id
        uint32_t agent;
        uint32_t client;
        char remote_name[32];
        struct databuffer buffer;
    };

    class ServiceGate : Context {
        int listen_id;
        uint32_t watchdog;
        uint32_t broker;
        int client_tag;
        int header_size;
        int max_connection;
        struct connection *conn;
    };
}



#endif //TINK_SERVICE_GATE_H
