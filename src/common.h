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
}

#endif //TINK_COMMON_H
