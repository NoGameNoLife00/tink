#pragma once

#include <cstdint>
#include <iconnection.h>
#include <atomic>
#include <msg.pb.h>

using namespace tink;
using namespace google;
namespace logic {
    class Player {
    public:
        int32_t pid;
        IConnectionPtr conn;
        float x;
        float y;
        float z;
        float v;
        explicit Player(IConnectionPtr conn);
        // 发送protobuf数据到客户端
        void SendMsg(int32_t msg_id, protobuf::Message& msg);
        // 同步玩家pid
        void SyncPid();
        // 广播玩家出生地点
        void BroadCastStartPosition();

        void Talk(const string &content);
    private:
        static std::atomic_int pid_gen;
    };

    typedef std::shared_ptr<Player> PlayerPtr;
}
