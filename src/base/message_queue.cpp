#include <message_queue.h>

namespace tink {
    void MessageQueue::Push(MsgPtr msg){
        std::lock_guard <Mutex> lock(mutex_);
        queue_.push(msg);
        if (!in_global) {
            GlobalMQInstance.Push(shared_from_this());
        }
        //��ʹ������ģʽ����Ϣ�����л�ȡ��Ϣʱ����condition������Ϣ����ʱ���ѵȴ��߳�
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
            //ע����һ�α������if����У���Ϊlock�������������if��������
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