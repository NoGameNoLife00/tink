#ifndef TINK_GLOBAL_MQ_H
#define TINK_GLOBAL_MQ_H
#include <list>
#include "net/message.h"
#include "base/message_queue.h"
#include "common.h"

//#define GLOBAL_MQ tink::Singleton<tink::GlobalMQ>::GetInstance()

namespace tink {
    class MessageQueue;

    typedef std::shared_ptr<MessageQueue> MQPtr;

    class GlobalMQ {
    public:
        void Push(MQPtr mq);
        MQPtr Pop();
    private:
        std::list<MQPtr> list;
        mutable Mutex mutex_;
    };

    std::shared_ptr<GlobalMQ> GetGlobalMQ();
}




#endif //TINK_GLOBAL_MQ_H
