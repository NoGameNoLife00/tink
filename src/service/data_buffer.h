#ifndef TINK_DATA_BUFFER_H
#define TINK_DATA_BUFFER_H

#include <array>
#include <list>

namespace tink::Service {
    constexpr auto MESSAGEPOOL = 1023;
    typedef struct Message_ {
        char *buffer;
        int size;
    } Message;
    typedef struct DataBuffer_ {
        int header;
        int offset;
        int size;
    } DataBuffer;

    class MessagePool {
        typedef std::array<Message, MESSAGEPOOL> MessageList;

    private:
        std::list<MessageList> pool;
        std::list<Message> free_list;
    };
}




#endif //TINK_DATA_BUFFER_H
