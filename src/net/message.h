#ifndef TINK_MESSAGE_H
#define TINK_MESSAGE_H

#include <leaked_object_detector.h>
#include <common.h>
#define GLOBALNAME_LENGTH 32
namespace tink {
    constexpr int MESSAGE_TYPE_MASK = (SIZE_MAX >> 8);
    constexpr int MESSAGE_TYPE_SHIFT = ((sizeof(size_t)-1) * 8);
    typedef struct NetMessage_ {
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
        LEAK_DETECTOR(NetMessage_);
    } NetMessage;

    typedef struct Message_ {
        uint32_t source;
        int32_t session;
        DataPtr data;
        size_t size;

        int Init(uint32_t source, int32_t session, BytePtr& data, size_t size);
    } Message ;

    typedef struct RemoteName_ {
        char name[GLOBALNAME_LENGTH];
        uint32_t handle;
    } RemoteName;

    typedef struct RemoteMessage_ {
        RemoteName destination;
        DataPtr message;
        size_t size;
        int type;
    } RemoteMessage;
    typedef std::shared_ptr<RemoteMessage> RemoteMsgPtr;
    typedef std::shared_ptr<Message> MsgPtr;
}



#endif //TINK_MESSAGE_H
