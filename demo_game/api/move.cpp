#include <move.h>
#include <msg.pb.h>
#include <spdlog/spdlog.h>
#include <msg_type.h>
#include <player.h>
#include <world_manager.h>

namespace api {
    int Move::Handle(tink::Request &request) {
        pb::Position msg;
        if (!msg.ParseFromArray(request.GetData().get(), request.GetDataLen())) {
            spdlog::warn("move position parse error");
            return E_UNPACK_FAILED;
        }
        std::string pid_str = request.GetConnection()->GetProperty(logic::PROP_PID);
        if (pid_str.empty()) {
            spdlog::warn("get property pid error");
            request.GetConnection()->Stop();
            return E_FAILED;
        }
        int pid = atoi(pid_str.c_str());
        spdlog::info("user pid={}, move({},{},{},{})", pid, msg.x(), msg.y(), msg.z(), msg.y());
        logic::PlayerPtr player = WorldMngInstance->GetPlayerByPid(pid);
        player->UpdatePos(msg.x(), msg.y(), msg.z(), msg.v());
        return E_OK;
    }

}
