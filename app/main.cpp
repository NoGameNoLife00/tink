#include <iostream>
#include <server.h>
#include <global_mng.h>
#include <sys/socket.h>
#include <base_router.h>
#include <cstring>
#include <error_code.h>
#include <message_handler.h>

class PingRouter : public tink::BaseRouter {
    int Handle(tink::IRequest &request) override {
        printf("call ping router [handle]\n");
        printf("recv from client: msgId = %d, data=%s\n", request.GetMsgId(), request.GetData().get());
        char *str = new char[20] {0};
        strcpy(str, "ping....\n");
        std::shared_ptr<byte> data(str);
        int e_code = request.GetConnection().SendMsg(1, data, strlen(str)+1);
        if (e_code != E_OK) {
            printf("send msg error:%d",e_code);
        }
        return E_OK;
    }

};

class HiRouter : public tink::BaseRouter {
    int Handle(tink::IRequest &request) override {
        printf("call hi router [handle]\n");
        printf("recv from client: msgId = %d, data=%s\n", request.GetMsgId(), request.GetData().get());
        char *str = new char[20] {0};
        strcpy(str, "ping....\n");
        std::shared_ptr<byte> data(str);
        int e_code = request.GetConnection().SendMsg(1, data, strlen(str)+1);
        if (e_code != E_OK) {
            printf("send msg error:%d",e_code);
        }
        return E_OK;
    }

};
int main() {
    setbuf(stdout, NULL); // debug
    std::shared_ptr<tink::GlobalMng> globalObj = GlobalInstance;
    globalObj->Init();

    tink::Server *s = new tink::Server();
    std::shared_ptr<tink::IRouter> br(new PingRouter());
    std::shared_ptr<tink::IRouter> hi_br(new HiRouter());
    std::shared_ptr<std::string> name(new std::string("tink"));
    std::shared_ptr<std::string> ip(new std::string("0.0.0.0"));
    std::shared_ptr<tink::IMessageHandler> handler(new tink::MessageHandler());
    s->Init(globalObj->GetName(), AF_INET, globalObj->GetHost(), globalObj->getPort(), handler);
    s->AddRouter(0, br);
    s->AddRouter(1, hi_br);
    s->Run();
    delete  s;
    return 0;
}
