//
// Created by admin on 2020/6/1.
//

#include "datapack.h"
#include "message.h"
#include <type.h>
#include <cstring>
#include <global_mng.h>
#include <error_code.h>

namespace tink {

    uint DataPack::GetHeadLen() {
        // uint32 + uint32
        return 8;
    }

    int DataPack::Pack(IMessage &msg, byte **data, uint *data_len) {
        *data_len = GetHeadLen() + msg.GetDataLen();
        *data = new byte[*data_len];
        byte * buff = *data;
        // 写id
        uint id = msg.GetId();
        memcpy(buff, &id, sizeof(id));
        // 写长度
        byte *ptr = buff + sizeof(id);
        uint len = msg.GetDataLen();
        memcpy(ptr, &len, sizeof(len));
        // 写data
        ptr += sizeof(len);
        memcpy(ptr, msg.GetData().get(), len);
//        data->reset(buff);
        return 0;
    }

    int DataPack::Unpack(byte *data, IMessage &msg) {
        byte *ptr = data;
        // 读id
        uint id = 0;
        memcpy(&id, ptr, sizeof(id));
        ptr = ptr + sizeof(id);
        // 读数据长度
        uint data_len = 0;
        memcpy(&data_len, ptr, sizeof(data_len));
//        ptr += sizeof(data_len);
        // 读数据
//        std::shared_ptr<byte> in_data(new byte[data_len]);
//        memcpy(in_data.get(), ptr, data_len);

        msg.SetId(id);
        msg.SetDataLen(data_len);
//        printf("unpack data id =%d", id);
//        printf("unpack data len =%d", data_len);
//        msg.SetData(in_data);
        std::shared_ptr<GlobalMng> globalObj = Singleton<GlobalMng>::GetInstance();
        if (globalObj->GetMaxPackageSize() > 0 && data_len > globalObj->GetMaxPackageSize()) {
            return E_PACKET_SIZE;
        }
        return E_OK;
    }

}
