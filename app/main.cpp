#include <iostream>
#include <server.h>
#include <global_mng.h>
#include <sys/socket.h>
#include <base_router.h>
#include <cstring>
#include <error_code.h>

class PingRouter : public tink::BaseRouter {
    int PreHandle(tink::IRequest &request) override {
        printf("call router [PreHandle]\n");
        int fd = request.GetConnection().GetTcpConn();
        char *str = "before ping\n";
        if (send(fd, str, strlen(str)+1, 0) == -1) {
            printf("call back ping error: %s\n", strerror(errno));
            return E_FAILED;
        }
        return E_OK;
    }

    int Handle(tink::IRequest &request) override {
        printf("call router [Handle]\n");
        int fd = request.GetConnection().GetTcpConn();
        char *str = "ping....\n";
        if (send(fd, str, strlen(str)+1, 0) == -1) {
            printf("call back ping error: %s\n", strerror(errno));
            return E_FAILED;
        }
        return E_OK;
    }

    int PostHandle(tink::IRequest &request) override {
        printf("call router [PostHandle]\n");
        int fd = request.GetConnection().GetTcpConn();
        char *str = "after ping\n";
        if (send(fd, str, strlen(str)+1, 0) == -1) {
            printf("call back ping error: %s\n", strerror(errno));
            return E_FAILED;
        }
        return E_OK;
    }
};

int main() {
    std::shared_ptr<tink::GlobalMng> globalObj = tink::Singleton<tink::GlobalMng>::GetInstance();
    globalObj->Init();

    tink::Server *s = new tink::Server();
    std::shared_ptr<PingRouter> br(new PingRouter());
    std::shared_ptr<std::string> name(new std::string("tink"));
    std::shared_ptr<std::string> ip(new std::string("0.0.0.0"));
    s->Init(globalObj->getName(), AF_INET, globalObj->getHost(), globalObj->getPort());
    s->AddRouter(br);
    s->Run();
    delete  s;
    return 0;
}
