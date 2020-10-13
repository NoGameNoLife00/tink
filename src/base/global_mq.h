#ifndef TINK_GLOBAL_MQ_H
#define TINK_GLOBAL_MQ_H
#include <singleton.h>
#include <common.h>
#include <list>
#include <message.h>
#include <message_queue.h>
#define GlobalMQInstance tink::Singleton<tink::GlobalMQ>::GetInstance()
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
}




#endif //TINK_GLOBAL_MQ_H
