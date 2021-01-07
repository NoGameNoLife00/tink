#ifndef TINK_MESSAGE_H
#define TINK_MESSAGE_H
#include "spdlog/spdlog.h"
#include "base/leaked_object_detector.h"
#include "common.h"


namespace tink {
    constexpr auto MESSAGE_TYPE_MASK = (SIZE_MAX >> 8);
    constexpr int MESSAGE_TYPE_SHIFT = ((sizeof(size_t)-1) * 8);
    constexpr int GLOBALNAME_LENGTH = 32;

    struct TinkMessage {
        uint32_t source;
        int32_t session;
        DataPtr data;
        size_t size;
        int Init(uint32_t _source, int32_t _session, BytePtr &_data, size_t _size);
        ~TinkMessage() {
            printf("~tink_message() data->%d:%d", data.use_count(), data.get());
        }
    };

    struct RemoteName {
        char name[GLOBALNAME_LENGTH];
        uint32_t handle;
    };

    struct RemoteMessage {
        RemoteName destination;
        DataPtr message;
        size_t size;
        int type;
    };
    typedef std::shared_ptr<RemoteMessage> RemoteMessagePtr;
    typedef std::shared_ptr<TinkMessage> TinkMessagePtr;
}



#endif //TINK_MESSAGE_H
