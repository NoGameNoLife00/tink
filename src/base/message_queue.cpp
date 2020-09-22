#include <message_queue.h>

namespace tink {
    void MessageQueue::Push(MsgPtr msg){
        std::lock_guard <Mutex> lock(mutex_);
        queue_.push(msg);
        if (!in_global) {
            GlobalMQInstance.Push(shared_from_this());
        }
        //当使用阻塞模式从消息队列中获取消息时，由condition在新消息到达时提醒等待线程
        condition_.notify_one();
    }

    MsgPtr MessageQueue::Pop(bool isBlocked){
        MsgPtr msg;
        if (isBlocked) {
            std::unique_lock <Mutex> lock(mutex_);
            while (queue_.empty())
            {
                condition_.wait(lock);
            }
            //注意这一段必须放在if语句中，因为lock的生命域仅仅在if大括号内
            msg = std::move(queue_.front());
            queue_.pop();
        } else {
            std::lock_guard<Mutex> lock(mutex_);
            if (queue_.empty())
                in_global = false;
                return nullptr;

            auto msg = std::move(queue_.front());
            queue_.pop();
        }
        return msg;
    }

    void MessageQueue::MarkRelease() {
        std::lock_guard<Mutex> lock(mutex_);
        assert(!release_);
        release_ = true;
        if (!in_global) {
            GlobalMQInstance.Push(shared_from_this());
        }
    }

    void MessageQueue::Release(MsgDrop drop_func, void *ud) {
        std::unique_lock<Mutex> lock(mutex_);
        if (release_) {
            lock.unlock();
            DropQueue_(drop_func, ud);
        } else {
            GlobalMQInstance.Push(shared_from_this());
        }
    }

    void MessageQueue::DropQueue_(MsgDrop drop_func, void *ud) {
        MsgPtr msg;
        while (msg = Pop(true)) {
            drop_func(msg, ud);
        }
    }
}