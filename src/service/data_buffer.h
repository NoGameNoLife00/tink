#ifndef TINK_DATA_BUFFER_H
#define TINK_DATA_BUFFER_H

#include <array>
#include <list>
#include <pool_set.h>
namespace tink::Service {
    constexpr auto MESSAGEPOOL = 1023;

    typedef struct Message_ {
        BytePtr buffer;
        int size;
    } Message;

    typedef PoolSet<Message> MessagePool;
    class DataBuffer {
    public:
        int header;
        int offset;
        int size;
        std::list<Message*> list;
        int ReadHeader(MessagePool& mp, int header_size);
        void Push(MessagePool& mp, void * data, int sz);
        void Read(MessagePool& mp, void * buffer, int sz);
        void Clear(MessagePool& mp);
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
