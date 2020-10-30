#include <message_queue.h>

namespace tink {
    void MessageQueue::Push(TinkMessage &msg){
        std::lock_guard <Mutex> lock(mutex_);
        queue_.push(msg);
        if (!in_global) {
            GLOBAL_MQ.Push(shared_from_this());
        }
        //当使用阻塞模式从消息队列中获取消息时，由condition在新消息到达时提醒等待线程
        condition_.notify_one();
    }

    bool MessageQueue::Pop(TinkMessage &msg, bool isBlocked){
        if (isBlocked) {
            std::unique_lock <Mutex> lock(mutex_);
            while (queue_.empty())
            {
                condition_.wait(lock);
            }
            //注意这一段必须放在if语句中，因为lock的生命域仅仅在if大括号内
            msg = queue_.front();
            queue_.pop();
        } else {
            std::lock_guard<Mutex> lock(mutex_);
            if (queue_.empty()) {
                in_global = false;
                return false;
            }
            msg = queue_.front();
            queue_.pop();
        }
        return true;
    }

    void MessageQueue::MarkRelease() {
        std::lock_guard<Mutex> lock(mutex_);
        assert(!release_);
        release_ = true;
        if (!in_global) {
            GLOBAL_MQ.Push(shared_from_this());
        }
    }

    void MessageQueue::Release(MsgDrop drop_func, void *ud) {
        std::unique_lock<Mutex> lock(mutex_);
        if (release_) {
            lock.unlock();
            DropQueue_(std::move(drop_func), ud);
        } else {
            GLOBAL_MQ.Push(shared_from_this());
        }
    }

    void MessageQueue::DropQueue_(MsgDrop drop_func, void *ud) {
        TinkMessage msg;
        while (Pop(msg, true)) {
            drop_func(msg, ud);
        }
    }
}