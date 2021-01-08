#ifndef TINK_GLOBAL_MQ_H
#define TINK_GLOBAL_MQ_H
#include <list>
#include "net/message.h"
#include "base/message_queue.h"
#include "common.h"

namespace tink {
    class MessageQueue;

    class GlobalMQ {
    public:
        using MsgQueuePtr = std::shared_ptr<MessageQueue>;
        void Push(MsgQueuePtr mq);
        MsgQueuePtr Pop();
    private:
        std::list<MsgQueuePtr> list;
        mutable Mutex mutex_;
    };
}




#endif //TINK_GLOBAL_MQ_H
