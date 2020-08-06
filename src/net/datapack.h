#ifndef TINK_DATAPACK_H
#define TINK_DATAPACK_H

#include <idatapack.h>
namespace tink {
    class DataPack: public IDataPack {
    public:
        uint32_t GetHeadLen();

        int Unpack(byte *data, IMessage &msg);

        int Pack(IMessage &msg, byte **data, uint32_t *data_len);
    };
}



#endif //TINK_DATAPACK_H
