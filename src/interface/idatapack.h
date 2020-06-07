//
// message序列化与反序列化接口
//

#ifndef TINK_IDATAPACK_H
#define TINK_IDATAPACK_H

#include <type.h>
#include "imessage.h"
namespace tink {
    // 处理TCP流的数据包
    class IDataPack {
    public:
        // 获取包的头的长度
        virtual uint GetHeadLen() = 0;
        // 序列化
        virtual int Pack(IMessage &msg, byte **data, uint *data_len) = 0;
        // 反序列化
        virtual int Unpack(byte *data, IMessage &msg) { return 0;};
    };

}



#endif //TINK_IDATAPACK_H
