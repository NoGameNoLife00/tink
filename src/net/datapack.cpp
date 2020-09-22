#include <datapack.h>
#include <common.h>
#include <cstring>
#include <global_mng.h>
#include <error_code.h>

namespace tink {

    uint32_t DataPack::GetHeadLen() {
        // uint32_t32 + uint32_t32
        return 8;
    }

    int DataPack::Pack(NetMessage &msg, BytePtr &data, uint32_t &data_len) {
        return Pack(msg, data.get(), data_len);
    }


    int DataPack::Pack(NetMessage &msg, byte *buff, uint32_t &data_len) {
        data_len = GetHeadLen() + msg.GetDataLen();
        // 写id
        uint32_t id = msg.GetId();
        memcpy(buff, &id, sizeof(id));
        // 写长度
        byte *ptr = buff + sizeof(id);
        uint32_t len = msg.GetDataLen();
        memcpy(ptr, &len, sizeof(len));
        // 写data
        ptr += sizeof(len);
        memcpy(ptr, msg.GetData().get(), len);
        return 0;
    }
    int DataPack::Unpack(BytePtr &data, NetMessage &msg) {
        byte *ptr = data.get();
        // 读id
        uint32_t id = 0;
        memcpy(&id, ptr, sizeof(id));
        ptr = ptr + sizeof(id);
        // 读数据长度
        uint32_t data_len = 0;
        memcpy(&data_len, ptr, sizeof(data_len));
        // 读数据
        msg.SetId(id);
        msg.SetDataLen(data_len);
        if (GlobalInstance.GetMaxPackageSize() > 0 && data_len > GlobalInstance.GetMaxPackageSize()) {
            return E_PACKET_SIZE;
        }
        return E_OK;
    }

}
