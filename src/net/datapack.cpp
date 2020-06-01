//
// Created by admin on 2020/6/1.
//

#include "datapack.h"
#include "message.h"
#include <type.h>
#include <cstring>
#include <global_mng.h>

namespace tink {

    uint DataPack::GetHeadLen() {
        // uint32 + uint32
        return 8;
    }

    int DataPack::Pack(std::shared_ptr<IMessage> msg, std::shared_ptr<byte> data) {
        uint buff_len = GetHeadLen() + msg->GetDataLen();
        byte * buff = new byte[buff_len];
        // 写id
        uint id = msg->GetId();
        memset(buff, id, sizeof(id));
        // 写长度
        byte *ptr = buff + sizeof(id);
        uint data_len = msg->GetDataLen();
        memset(ptr, data_len, sizeof(data_len));
        // 写data
        ptr += sizeof(data_len);
        memcpy(ptr, msg->GetData().get(), data_len);
        data.reset(buff);
        return 0;
    }

    int DataPack::Unpack(std::shared_ptr<byte> data, std::shared_ptr<IMessage> msg) {
        byte *ptr = data.get();
        // 读id
        uint id = 0;
        memcpy(&id, ptr, sizeof(id));
        ptr += sizeof(id);
        // 读数据长度
        uint data_len = 0;
        memcpy(&data_len, ptr, sizeof(data_len));
        ptr += sizeof(data_len);
        // 读数据
        std::shared_ptr<byte> in_data(new byte[data_len]);
        memcpy(in_data.get(), ptr, data_len);

        msg->SetId(id);
        msg->SetDataLen(data_len);
        msg->SetData(in_data);
        std::shared_ptr<GlobalMng> globalObj = Singleton<GlobalMng>::GetInstance();
        return 0;
    }

}
