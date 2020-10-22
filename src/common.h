#ifndef TINK_COMMON_H
#define TINK_COMMON_H
#include <sys/types.h>
#include <memory>
#include <mutex>
#include <vector>



namespace tink {
    typedef char byte;
    typedef std::shared_ptr<void> DataPtr;
    typedef std::unique_ptr<byte[]> UBytePtr;
    typedef std::shared_ptr<byte[]> BytePtr;
    typedef std::shared_ptr<std::string> StringPtr;
    typedef std::mutex Mutex;

    constexpr static int HANDLE_REMOTE_SHIFT = 24;
    constexpr static int HANDLE_MASK = 0xffffff;

    constexpr auto PTYPE_TEXT = 0;
    constexpr auto PTYPE_RESPONSE = 1;
    constexpr auto PTYPE_CLIENT = 3;
    constexpr auto PTYPE_SYSTEM = 4;
    constexpr auto PTYPE_HARBOR = 5;
    constexpr auto PTYPE_SOCKET = 6;
    constexpr auto PTYPE_ERROR = 7;
    constexpr auto PTYPE_TAG_DONTCOPY = 0x10000;
    constexpr auto PTYPE_TAG_ALLOCSESSION = 0x20000;


    constexpr auto THREAD_WORKER = 0;
    constexpr auto THREAD_MAIN = 1;
    constexpr auto THREAD_SOCKET = 2;
    constexpr auto THREAD_TIMER = 3;
    constexpr auto THREAD_MONITOR = 4;


    template< typename T >
    struct ArrayDeleter
    {
        void operator ()( T const * p)
        {
            delete[] p;
        }
    };

}

#endif //TINK_COMMON_H
