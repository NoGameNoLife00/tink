#ifndef TINK_MESSAGE_QUEUE_H
#define TINK_MESSAGE_QUEUE_H

#include <functional>
#include <queue>
#include <condition_variable>
#include "base/global_mq.h"
#include "base/noncopyable.h"
#include "common.h"

namespace tink {
    class GlobalMQ;

    class MessageQueue : public noncopyable, std::enable_shared_from_this<MessageQueue> {
    public:
        using MsgDrop = std::function<void(TinkMessage&, void *)>;
        MessageQueue(GlobalMQ* global_mq,uint32_t handle);
        virtual ~MessageQueue(){}
        void Push(TinkMessage &msg);
        //blocked定义访问方式是同步阻塞或者非阻塞模式
        bool Pop(TinkMessage &msg, bool isBlocked = true);

        int32_t Size(){
            std::lock_guard<Mutex> lock(mutex_);
            return queue_.size();
        }

        uint32_t Handle() const {return handle_;}

        bool Empty(){
            std::lock_guard<Mutex> lock(mutex_);
            return queue_.empty();
        }

        void MarkRelease();
        void Release(MsgDrop drop_func, void* ud);
    private:
        void DropQueue_(MsgDrop drop_func, void *ud);
        std::queue<TinkMessage> queue_;//存储消息的队列
        mutable Mutex mutex_;//同步锁
        std::condition_variable condition_;//实现同步式获取消息
        uint32_t handle_;
        bool release_;
        bool in_global_;
        GlobalMQ* global_mq_;
    };
    using MsgQueuePtr = std::shared_ptr<MessageQueue>;
}
#endif //TINK_MESSAGE_QUEUE_H
