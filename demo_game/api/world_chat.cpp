//
// Created by Ёблн on 2020/9/9.
//

#include "world_chat.h"
#include <msg.pb.h>
#include <spdlog/spdlog.h>
#include <msg_type.h>
#include <player.h>
#include <world_manager.h>

namespace api {
    int WorldChat::Handle(tink::IRequest &request) {
        pb::Talk msg;
        if (!msg.ParseFromArray(request.GetData().get(), request.GetDataLen())) {
            spdlog::warn("talk parse error");
            return E_UNPACK_FAILED;
        }
        std::string pid_str = request.GetConnection()->GetProperty(logic::PROP_PID);
        if (pid_str.empty()) {
            spdlog::warn("get property pid error");
            return E_FAILED;
        }
        int pid = atoi(pid_str.c_str());
        logic::PlayerPtr player = WorldMngInstance->GetPlayerByPid(pid);
        if (player) {
            player.Talk();
        }
        return BaseRouter::Handle(request);

    }

}
