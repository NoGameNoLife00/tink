#ifndef TINK_CONFIG_H
#define TINK_CONFIG_H
#include <singleton.h>
#include <common.h>
#include <list>
#include <message.h>
#include <message_queue.h>
#define GLOBAL_MQ tink::Singleton<tink::GlobalMQ>::GetInstance()
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




#endif //TINK_CONFIG_H
