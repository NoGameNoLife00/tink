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

        int Unpack(std::shared_ptr<byte> data, std::shared_ptr<IMessage> msg) override;

        int Pack(std::shared_ptr<IMessage> msg, std::shared_ptr<byte> *data);
    };
}



#endif //TINK_DATAPACK_H
