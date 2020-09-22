#ifndef TINK_GLOBAL_MQ_H
#define TINK_GLOBAL_MQ_H
#include <singleton.h>
#include <type.h>
#include <list>
#include <message_queue.h>
#include <message.h>

namespace tink {
    class GlobalMQ : public Singleton<GlobalMQ>  {
    public:
        void Push(MQPtr mq);
        MQPtr Pop();
    private:
        std::list<MQPtr> list;
        mutable Mutex mutex_;
    };
}




#endif //TINK_GLOBAL_MQ_H
