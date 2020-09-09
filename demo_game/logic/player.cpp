#include "player.h"
#include "world_manager.h"
#include <msg_type.h>
#include <global_mng.h>

namespace logic {
    std::atomic_int Player::pid_gen(1);

    int Random(int x, int y) {
        return  (rand() % (y-x+1)) + x;
    }

    Player::Player(IConnectionPtr conn) : conn(conn){
        pid = pid_gen.fetch_add(1);
        x = 160 + Random(0, 10);
        y = 0;
        z = 140 + Random(0, 20);
        v = 0;
    }

    void Player::SendMsg(int32_t msg_id, protobuf::Message &msg) {
        int size = msg.ByteSizeLong();
        BytePtr data = std::make_unique<byte[]>(size);
        spdlog::info("msg data = %v", msg.DebugString());
        if (!msg.SerializeToArray(data.get(), size)) {
            return;
        }
        int ret = conn->SendMsg(msg_id, data, size);
        if (ret != E_OK) {
            spdlog::warn("player send msg error");
        }
    }

    void Player::SyncPid() {
        pb::SyncPid data;
        data.set_pid(pid);
        SendMsg(MSG_SYNC_PID, data);
    }

    void Player::BroadCastStartPosition() {
        pb::BroadCast data;
        pb::Position *pos = new pb::Position();
        pos->set_x(x);
        pos->set_y(y);
        pos->set_v(v);
        pos->set_z(z);
        data.set_pid(pid);
        data.set_tp(2); // ¹ã²¥: 2-Î»ÖÃ×ø±ê
        data.set_allocated_p(pos);
        SendMsg(MSG_BROADCAST_POS, data);
    }

    void Player::Talk(const string &content) {
        pb::BroadCast msg;
        msg.set_pid(pid);
        msg.set_content(content);
        PlayerList players;
        WorldMngInstance->GetAllPlayers(players);
        for (auto& player: players) {
            player->SendMsg(MSG_BROADCAST_POS, msg);
        }
    }

}

