#include <iostream>
#include <server.h>
#include <global_mng.h>
#include <sys/socket.h>
#include <base_router.h>
#include <cstring>
#include <error_code.h>
#include <message_handler.h>
//#include <easylogging++.h>
#include <iconnection.h>
#include <player.h>

class PingRouter : public tink::BaseRouter {
    int Handle(tink::IRequest &request) override {
        printf("call ping router [handle]\n");
        printf("recv from client: msgId = %d, data=%s\n", request.GetMsgId(), request.GetData().get());
        char *str = new char[20] {0};
        strcpy(str, "ping....\n");
        BytePtr data(str);
        int e_code = request.GetConnection()->SendMsg(1, data, strlen(str)+1);
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
        BytePtr data(str);
        int e_code = request.GetConnection()->SendMsg(1, data, strlen(str)+1);
        if (e_code != E_OK) {
            printf("send msg error:%d",e_code);
        }
        return E_OK;
    }

};

void DoConnectionAdd(tink::IConnectionPtr& conn) {
    logic::Player player(conn);
    player.SyncPid();
    player.BroadCastStartPosition();
//    LOG(INFO) << "player pid = " << player.pid << "online";
}


void DoConnectionLost(tink::IConnectionPtr& conn) {
    printf("do_connection_lost is called...\n");
}

int main(int argc, char** argv) {
    START_EASYLOGGINGPP(argc, argv);
    setbuf(stdout, NULL); // debug
    srand(static_cast<unsigned>(time(NULL)));
    auto globalObj = GlobalInstance;
    globalObj->Init();

    std::shared_ptr<tink::Server> s(new tink::Server());

    tink::IRouterPtr br(new PingRouter());
    tink::IRouterPtr hi_br(new HiRouter());
    StringPtr name(new std::string("demo game tink"));
    StringPtr ip(new std::string("0.0.0.0"));

    std::shared_ptr<tink::MessageHandler>  handler(new tink::MessageHandler());

    globalObj->SetServer(std::dynamic_pointer_cast<tink::IServer>(s));
    handler->Init();
    s->Init(const_cast<std::string &>(globalObj->GetName()), AF_INET,
            const_cast<std::string &>(globalObj->GetHost()), globalObj->GetPort(),
            std::dynamic_pointer_cast<tink::IMessageHandler>(handler));
    s->SetOnConnStop(&DoConnectionLost);
    s->SetOnConnStart(&DoConnectionAdd);
    s->AddRouter(0, br);
    s->AddRouter(1, hi_br);
    s->Run();
    return 0;
}
