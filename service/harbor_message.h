#ifndef TINK_HARBOR_MESSAGE_H
#define TINK_HARBOR_MESSAGE_H

#include <sys/types.h>
#include <string>
#include <list>
#include <memory>
#include <unordered_map>
#include "common.h"

namespace tink::Service {
    struct RemoteMsgHeader {
        uint32_t source;
        uint32_t destination;
        uint32_t session;
    };

    struct HarborMsg {
        RemoteMsgHeader header;
        DataPtr buffer;
        size_t size;
    };

    typedef std::shared_ptr<HarborMsg> HarborMsgPtr;
    typedef std::shared_ptr<RemoteMsgHeader> RemoteMsgHeaderPtr;
    typedef std::list<HarborMsg> HarborMsgQueue;
    typedef std::shared_ptr<HarborMsgQueue> HarborMsgQueuePtr;
    typedef std::pair<int32_t, HarborMsgQueuePtr> HarborValue;
    typedef std::unordered_map<std::string, HarborValue> HarborMap;

     inline void ToBigEndian(uint8_t *buffer, uint32_t n) {
        buffer[0] = (n >> 24) & 0xff;
        buffer[1] = (n >> 16) & 0xff;
        buffer[2] = (n >> 8) & 0xff;
        buffer[3] = n & 0xff;
    }

    inline uint32_t FromBigEndian(uint32_t n) {
        union {
            uint32_t big;
            uint8_t bytes[4];
        } u{};
        u.big = n;
        return u.bytes[0] << 24 | u.bytes[1] << 16 | u.bytes[2] << 8 | u.bytes[3];
    }

    inline void HeaderToMessage(const RemoteMsgHeader &header, uint8_t *message) {
        ToBigEndian(message, header.source);
        ToBigEndian(message + 4, header.destination);
        ToBigEndian(message + 8, header.session);
    }

    inline void MessageToHeader(const uint32_t *message, RemoteMsgHeader &header) {
        header.source = FromBigEndian(message[0]);
        header.destination = FromBigEndian(message[1]);
        header.session = FromBigEndian(message[2]);
    }

}


#endif //TINK_HARBOR_MESSAGE_H
