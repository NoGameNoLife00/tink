#ifndef TINK_DATA_BUFFER_H
#define TINK_DATA_BUFFER_H

#include <array>
#include <list>
#include "base/pool_set.h"
#include "common.h"

namespace tink::Service {
    constexpr auto MESSAGEPOOL = 1023;

    struct Message {
        DataPtr buffer;
        int size;
    };

    typedef PoolSet<Message> MessagePool;
    class DataBuffer {
    public:
        int header;
        int offset;
        int size;
        std::list<Message*> list;
        int ReadHeader(MessagePool& mp, int header_size);
        void Push(MessagePool& mp, DataPtr data, int sz);
        void Read(MessagePool& mp, void *buffer, int sz);
        void Clear(MessagePool& mp);
        void Reset() { header = 0; }
    private:
        void ReturnMessage_(MessagePool &mp);
    } ;




//    class MessagePool {
//        typedef std::array<Message, MESSAGEPOOL> MessageList;
//
//    private:
//        std::list<MessageList> pool;
//        std::list<Message> free_list;
//    };
}




#endif //TINK_DATA_BUFFER_H
