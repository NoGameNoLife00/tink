
#ifndef TINK_MESSAGE_HANDLER_H
#define TINK_MESSAGE_HANDLER_H


#include <map>
#include <irouter.h>
#include <imessage_handler.h>
#include <vector>
#include <message_queue.h>
#include <irequest.h>

namespace tink {

    typedef MessageQueue<IRequestPtr> IRequestMsgQueue;
    typedef std::shared_ptr<IRequestMsgQueue> IRequestMsgQueuePtr;
    typedef std::vector<IRequestMsgQueuePtr> MsgQueueList;

    class MessageHandler : public IMessageHandler {
    public:

//        virtual ~MessageHandler();
        // 存放每个MsgId对应的方法
        std::map<uint32_t, std::shared_ptr<IRouter>> apis;
        // 消息队列
        MsgQueueList task_queue;
        // worker 池数量
        uint32_t worker_pool_size;
        // worker pid
        std::vector<pthread_t> worker_pid_list;

        int Init();
        // 调度执行Router消息处理方法
        virtual int DoMsgHandle(IRequest &request);
        // 绑定Message对应的Router
        virtual int AddRouter(uint32_t msg_id, IRouterPtr &router);

        virtual int StartWorkerPool();
        static void* StartOneWorker(void* worker_info_ptr);
        // 将消息发送给任务队列
        int SendMsgToTaskQueue(IRequestPtr &request);
    };

    typedef struct WorkerInfo_ {
        MessageHandler * msg_handler;
        int worker_id;
    } WorkerInfo;
}

#endif //TINK_MESSAGE_HANDLER_H