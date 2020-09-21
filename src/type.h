#ifndef TINK_TYPE_H
#define TINK_TYPE_H
#include <sys/types.h>
#include <memory>
#include <mutex>
#include <vector>
#include <functional>
#include <context.h>
namespace tink {
    class Context;
    typedef char byte;
    typedef std::unique_ptr<byte[]> BytePtr;
    typedef std::shared_ptr<std::string> StringPtr;
    typedef std::shared_ptr<struct sockaddr> RemoteAddrPtr;
    typedef std::mutex Mutex;
    typedef std::function<int (Context& ctx, void* ud, int type, int session, uint32_t source, const void* msg, size_t sz)> ContextCallBack;
}

#endif //TINK_TYPE_H
