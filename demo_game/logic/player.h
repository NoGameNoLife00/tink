#pragma once

#include <cstdint>
#include <iconnection.h>
#include <atomic>
#include <msg.pb.h>

using namespace tink;
using namespace google;
namespace logic {
    class Player;
    typedef std::shared_ptr<Player> PlayerPtr;
    typedef std::vector<PlayerPtr> PlayerList;
    typedef std::vector<int32_t> PidList;
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
        // 聊天
        void Talk(const string &content);
        // 更新玩家位置
        void UpdatePos(float x, float y, float z, float v);
        // 获取当前玩家AOI周边玩家
        void GetSurroundingPlayers(PlayerList& players);
        // 玩家下线
        void LostConnection();
    private:
        static std::atomic_int pid_gen;
    };
}
