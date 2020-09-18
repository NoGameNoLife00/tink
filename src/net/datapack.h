#ifndef TINK_DATAPACK_H
#define TINK_DATAPACK_H

#include <type.h>
#include <message.h>
namespace tink {
    class DataPack {
    public:
        // ��ȡ����ͷ�ĳ���
        static uint32_t GetHeadLen();
        // �����л�
        static int Unpack(BytePtr &data, NetMessage &msg);
        // ���л�
        static int Pack(NetMessage &msg, BytePtr &data, uint32_t &data_len);

        static int Pack(NetMessage &msg, byte *buff, uint32_t &data_len);
    };
}



#endif //TINK_DATAPACK_H
