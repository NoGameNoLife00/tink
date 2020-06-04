//
// Created by admin on 2020/6/1.
//

#ifndef TINK_DATAPACK_H
#define TINK_DATAPACK_H

#include <idatapack.h>
namespace tink {
    class DataPack: public IDataPack {
    public:
        uint GetHeadLen();

        int Unpack(byte *data, IMessage &msg);

        int Pack(IMessage &msg, byte *data);
    };
}



#endif //TINK_DATAPACK_H
