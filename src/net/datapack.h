#ifndef TINK_DATAPACK_H
#define TINK_DATAPACK_H

#include <type.h>
#include <message.h>
namespace tink {
    class DataPack {
    public:
        // 获取包的头的长度
        static uint32_t GetHeadLen();
        // 反序列化
        static int Unpack(BytePtr &data, NetMessage &msg);
        // 序列化
        static int Pack(NetMessage &msg, BytePtr &data, uint32_t &data_len);

        static int Pack(NetMessage &msg, byte *buff, uint32_t &data_len);
    };
}



#endif //TINK_DATAPACK_H
