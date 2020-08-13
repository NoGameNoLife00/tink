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
        virtual int32_t GetId() const = 0;
        virtual void SetId(uint32_t id) {};
        virtual uint32_t GetDataLen() const {return 0;};
        virtual void SetDataLen(uint32_t dataLen) {};
        virtual BytePtr & GetData() = 0;
        virtual void SetData(BytePtr &data) {};
//        virtual int Init(uint32_t id, uint32_t len, const std::shared_ptr<byte> &data);
        virtual ~IMessage() {};
    };
    typedef std::unique_ptr<IMessage> IMessagePtr;
}

#endif //TINK_IMESSAGE_H
