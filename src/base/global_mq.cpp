#include "base/global_mq.h"
namespace tink {
    void GlobalMQ::Push(MQPtr mq) {
        std::lock_guard<Mutex> lock(mutex_);
        list.emplace_back(mq);
    }

    MQPtr GlobalMQ::Pop() {
        std::lock_guard<Mutex> lock(mutex_);
        if (list.size() > 0) {
            auto&& head = list.front();
            list.pop_front();
            return head;
        }
        return nullptr;
    }

}
