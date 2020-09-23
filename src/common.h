#ifndef TINK_COMMON_H
#define TINK_COMMON_H
#include <sys/types.h>
#include <memory>
#include <mutex>
#include <vector>



namespace tink {
    typedef char byte;
    typedef std::unique_ptr<byte[]> BytePtr;
    typedef std::shared_ptr<std::string> StringPtr;
    typedef std::shared_ptr<struct sockaddr> RemoteAddrPtr;
    typedef std::mutex Mutex;

    constexpr auto PTYPE_TEXT = 0;
    constexpr auto PTYPE_RESPONSE = 1;
    constexpr auto PTYPE_CLIENT = 3;
    constexpr auto PTYPE_SYSTEM = 4;
    constexpr auto PTYPE_HARBOR = 5;
    constexpr auto PTYPE_SOCKET = 6;
    constexpr auto PTYPE_ERROR = 7;
    constexpr auto PTYPE_TAG_ALLOCSESSION = 0x20000;
}

#endif //TINK_COMMON_H
