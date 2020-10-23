#ifndef TINK_MESSAGE_H
#define TINK_MESSAGE_H

#include <leaked_object_detector.h>
#include <common.h>

namespace tink {
    constexpr auto MESSAGE_TYPE_MASK = (SIZE_MAX >> 8);
    constexpr int MESSAGE_TYPE_SHIFT = ((sizeof(size_t)-1) * 8);
    constexpr int GLOBALNAME_LENGTH = 32;

    typedef struct TinkMessage_ {
        uint32_t source;
        int32_t session;
        DataPtr data;
        size_t size;
        int Init(uint32_t _source, int32_t _session, UBytePtr& _data, size_t _size);
    } TinkMessage ;

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
    typedef std::shared_ptr<RemoteMessage> RemoteMessagePtr;
    typedef std::shared_ptr<TinkMessage> TinkMessagePtr;
}



#endif //TINK_MESSAGE_H
