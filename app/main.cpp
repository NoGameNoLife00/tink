#include <iostream>
#include <server.h>
#include <cygwin/socket.h>
#include <base_router.h>
#include <sys/socket.h>
#include <cstring>
#include <error_code.h>
#include <w32api/asptlb.h>

class PingRouter : public tink::BaseRouter {
    int PreHandle(tink::IRequest &request) override {
        printf("call router [PreHandle]\n");
        int fd = request.GetConnection()->GetTcpConn();
        char *str = "before ping\n";
        if (send(fd, str, strlen(str)+1, 0) == -1) {
            printf("call back ping error: %s\n", strerror(errno));
            return E_FAILED;
        }
        return E_OK;
    }

    int Handle(tink::IRequest &request) override {
        printf("call router [Handle]\n");
        int fd = request.GetConnection()->GetTcpConn();
        char *str = "ping....\n";
        if (send(fd, str, strlen(str)+1, 0) == -1) {
            printf("call back ping error: %s\n", strerror(errno));
            return E_FAILED;
        }
        return E_OK;
    }

    int PostHandle(tink::IRequest &request) override {
        printf("call router [PostHandle]\n");
        int fd = request.GetConnection()->GetTcpConn();
        char *str = "after ping\n";
        if (send(fd, str, strlen(str)+1, 0) == -1) {
            printf("call back ping error: %s\n", strerror(errno));
            return E_FAILED;
        }
        return E_OK;
    }
};

int main() {
    tink::Server *s = new tink::Server();
    PingRouter *br = new PingRouter();
    s->Init("tink", AF_INET, "0.0.0.0", 8823);
    s->AddRouter(br);
    s->Run();
    delete  s;
    delete br;
    return 0;
}
