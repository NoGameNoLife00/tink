//
// 请求消息接口
//

#ifndef TINK_IMESSAGE_H
#define TINK_IMESSAGE_H

#include <type.h>
#include <memory>

namespace tink {
    class IMessage {
    public:
        virtual uint GetId() const = 0;
        virtual void SetId(uint id) {};
        virtual uint GetDataLen() const {return 0;};
        virtual void SetDataLen(uint dataLen) {};
        virtual std::shared_ptr<byte>& GetData() = 0;
        virtual void SetData(const std::shared_ptr<byte> &data) {};
    };
}

#endif //TINK_IMESSAGE_H
