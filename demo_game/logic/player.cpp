#include "player.h"

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
        std::string data;
        if (!msg.SerializeToString(&data)) {
            return;
        }
        
    }

}

