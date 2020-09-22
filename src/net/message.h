#ifndef TINK_MESSAGE_H
#define TINK_MESSAGE_H

#include <leaked_object_detector.h>
#include <type.h>
namespace tink {
    struct NetMessage {
        int Init(uint32_t id, uint32_t len, BytePtr &data);
        int32_t GetId() const;

        void SetId(uint32_t id);

        uint32_t GetDataLen() const;

        void SetDataLen(uint32_t data_len);

        BytePtr &GetData();

        void SetData(BytePtr &data);
        uint32_t id; // 消息ID
        uint32_t data_len; // 消息长度
        BytePtr data; // 消息数据
        LEAK_DETECTOR(NetMessage);
    };

    struct Message {
        uint32_t source;
        int32_t session;
        BytePtr data;
        size_t size;

        int Init(uint32_t source, int32_t session, BytePtr& data, size_t size);
    };

    typedef std::unique_ptr<Message> MsgPtr;
}



#endif //TINK_MESSAGE_H
