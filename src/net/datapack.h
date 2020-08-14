#ifndef TINK_DATAPACK_H
#define TINK_DATAPACK_H

#include <idatapack.h>
namespace tink {
    class DataPack {
    public:
        static uint32_t GetHeadLen();

        static int Unpack(BytePtr &data, IMessage &msg);

        static int Pack(IMessage &msg, BytePtr &data, uint32_t &data_len);
    };
}



#endif //TINK_DATAPACK_H
