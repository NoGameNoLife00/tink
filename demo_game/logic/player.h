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
        // ����protobuf���ݵ��ͻ���
        void SendMsg(int32_t msg_id, protobuf::Message& msg);
        // ͬ�����pid
        void SyncPid();
        // �㲥��ҳ����ص�
        void BroadCastStartPosition();
        // ����
        void Talk(const string &content);
        // �������λ��
        void UpdatePos(float x, float y, float z, float v);
        // ��ȡ��ǰ���AOI�ܱ����
        void GetSurroundingPlayers(PlayerList& players);
        // �������
        void LostConnection();
    private:
        static std::atomic_int pid_gen;
    };
}
