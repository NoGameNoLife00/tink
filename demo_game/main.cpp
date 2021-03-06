#include <iostream>
#include <server.h>
#include <config.h>
#include <cstring>
#include <message_handler.h>
#include <connection.h>
#include <player.h>
#include <world_manager.h>
#include <msg_type.h>
#include <world_chat.h>
#include <move.h>

void DoConnectionAdd(tink::ConnectionPtr& conn) {
    logic::PlayerPtr player = std::make_shared<logic::Player>(conn);
    player->SyncPid();
    player->BroadCastStartPosition();
    WorldMngInstance.AddPlayer(player);
    conn->SetProperty(logic::PROP_PID, std::to_string(player->pid));
    spdlog::info("player pid = {} online...", player->pid);
}


void DoConnectionLost(tink::ConnectionPtr& conn) {
    string pid_str = conn->GetProperty(logic::PROP_PID);
    if (pid_str.empty()) {
        spdlog::warn("get property pid error");
        return;
    }
    int pid = atoi(pid_str.c_str());
    logic::PlayerPtr player = WorldMngInstance.GetPlayerByPid(pid);
    if (player) {
        player->LostConnection();
    }
    spdlog::info("player pid= {} offline...", pid);
}

int main(int argc, char** argv) {
    setbuf(stdout, NULL); // debug
    srand(static_cast<unsigned>(time(NULL)));
    auto& globalObj = ConfigMngInstance;
    globalObj.Init();

    std::shared_ptr<tink::Server> s(new tink::Server());

    tink::BaseRouterPtr chat_api(new api::WorldChat());
    tink::BaseRouterPtr move_api(new api::Move());
    StringPtr name(new std::string("demo game tink"));
    StringPtr ip(new std::string("0.0.0.0"));

    std::shared_ptr<tink::MessageHandler>  handler(new tink::MessageHandler());

    globalObj.SetServer(std::dynamic_pointer_cast<tink::Server>(s));
    handler->Init();
    s->Init(const_cast<std::string &>(globalObj.GetName()), AF_INET,
            const_cast<std::string &>(globalObj.GetHost()), globalObj.GetPort());
    WorldMngInstance.Init();
    s->Start();
//    spdlog::drop_all();
    return 0;
}
