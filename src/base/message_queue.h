#ifndef TINK_MESSAGE_QUEUE_H
#define TINK_MESSAGE_QUEUE_H


#include <queue>
#include <type.h>
#include <condition_variable>
#include "global_mq.h"

namespace tink {


    template<class Type>
    class MessageQueueT {
    public:
        MessageQueueT& operator = (const MessageQueueT&) = delete;
        MessageQueueT(const MessageQueueT& mq) = delete;
        MessageQueueT() : queue_(), mutex_(), condition_(){}
        virtual ~MessageQueueT(){}
        void Push(Type msg){
            std::lock_guard <Mutex> lock(mutex_);
            queue_.push(msg);
            //当使用阻塞模式从消息队列中获取消息时，由condition在新消息到达时提醒等待线程
            condition_.notify_one();
        }

        //blocked定义访问方式是同步阻塞或者非阻塞模式
        bool Pop(Type& msg, bool isBlocked = true){
            if (isBlocked)
            {
                std::unique_lock <Mutex> lock(mutex_);
                while (queue_.empty())
                {
                    condition_.wait(lock);

                }
                //注意这一段必须放在if语句中，因为lock的生命域仅仅在if大括号内
                msg = std::move(queue_.front());
                queue_.pop();
                return true;

            }
            else
            {
                std::lock_guard<Mutex> lock(mutex_);
                if (queue_.empty())
                    return false;

                msg = std::move(queue_.front());
                queue_.pop();
                return true;
            }

        }

        int32_t Size(){
            std::lock_guard<Mutex> lock(mutex_);
            return queue_.size();
        }

        bool Empty(){
            std::lock_guard<Mutex> lock(mutex_);
            return queue_.empty();
        }
    private:
        std::queue<Type> queue_;//存储消息的队列
        mutable Mutex mutex_;//同步锁
        std::condition_variable condition_;//实现同步式获取消息
    };

    typedef std::function<void(MsgPtr, void *)> MsgDrop;
    class MessageQueue : public std::enable_shared_from_this<MessageQueue> {
    public:
        MessageQueue& operator = (const MessageQueue&) = delete;
        MessageQueue(const MessageQueue& mq) = delete;
        MessageQueue(uint32_t handle) : handle_(handle), in_global(true),
                                        release_(false), queue_(), mutex_(), condition_() {}
        virtual ~MessageQueue(){}
        void Push(MsgPtr msg);
        //blocked定义访问方式是同步阻塞或者非阻塞模式
        MsgPtr Pop(bool isBlocked = true);

        int32_t Size(){
            std::lock_guard<Mutex> lock(mutex_);
            return queue_.size();
        }

        uint32_t Handle() {return handle_;}

        bool Empty(){
            std::lock_guard<Mutex> lock(mutex_);
            return queue_.empty();
        }

        void MarkRelease();
        void Release(MsgDrop drop_func, void* ud);
    private:
        void DropQueue_(MsgDrop drop_func, void *ud);
        std::queue<MsgPtr> queue_;//存储消息的队列
        mutable Mutex mutex_;//同步锁
        std::condition_variable condition_;//实现同步式获取消息
        uint32_t handle_;
        bool release_;
        bool in_global;
    };
    typedef std::shared_ptr<MessageQueue> MQPtr;

}
#endif //TINK_MESSAGE_QUEUE_H
