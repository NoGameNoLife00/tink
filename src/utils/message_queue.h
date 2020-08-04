//
// Created by admin on 2020/7/15.
//

#ifndef TINK_MESSAGE_QUEUE_H
#define TINK_MESSAGE_QUEUE_H
#include <queue>
#include <mutex>
#include <condition_variable>

namespace tink {
    template<class Type>
    class MessageQueue {
    public:
        MessageQueue& operator = (const MessageQueue&) = delete;
        MessageQueue(const MessageQueue& mq) = delete;
        MessageQueue() : queue_(), mutex_(), condition_(){}
        virtual ~MessageQueue(){}
        void Push(Type msg){
            std::lock_guard <std::mutex> lock(mutex_);
            queue_.push(msg);
            //当使用阻塞模式从消息队列中获取消息时，由condition在新消息到达时提醒等待线程
            condition_.notify_one();
        }

        //blocked定义访问方式是同步阻塞或者非阻塞模式
        bool Pop(Type& msg, bool isBlocked = true){
            if (isBlocked)
            {
                std::unique_lock <std::mutex> lock(mutex_);
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
                std::lock_guard<std::mutex> lock(mutex_);
                if (queue_.empty())
                    return false;


                msg = std::move(queue_.front());
                queue_.pop();
                return true;
            }

        }

        int32_t Size(){
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.size();
        }

        bool Empty(){
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.empty();
        }
    private:
        std::queue<Type> queue_;//存储消息的队列
        mutable std::mutex mutex_;//同步锁
        std::condition_variable condition_;//实现同步式获取消息
    };
}




#endif //TINK_MESSAGE_QUEUE_H