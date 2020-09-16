
#ifndef TINK_MESSAGE_HANDLER_H
#define TINK_MESSAGE_HANDLER_H

#include <map>
#include <base_router.h>
#include <vector>
#include <message_queue.h>
#include <request.h>
#include <thread.h>

namespace tink {
    class Request;
    class BaseRouter;
    typedef std::shared_ptr<Request> RequestPtr;
    typedef std::shared_ptr<BaseRouter> BaseRouterPtr;
    class MessageHandler {
    public:
        typedef MessageQueue<RequestPtr> RequestMsgQueue;
        typedef std::shared_ptr<RequestMsgQueue> RequestMsgQueuePtr;
        typedef std::vector<RequestMsgQueuePtr> MsgQueueList;
//        virtual ~MessageHandler();
        // 存放每个MsgId对应的方法
        std::map<uint32_t, std::shared_ptr<BaseRouter>> apis;
        // 消息队列
        MsgQueueList task_queue;
        // worker 池数量
        uint32_t worker_pool_size;

        int Init();
        // 调度执行Router消息处理方法
        virtual int DoMsgHandle(Request &request);
        // 绑定Message对应的Router
        virtual int AddRouter(uint32_t msg_id, BaseRouterPtr &router);

        virtual int StartWorkerPool();

        static void StartOneWorker(MessageHandler& handler, int worker_id);
        // 将消息发送给任务队列
        int SendMsgToTaskQueue(RequestPtr &request);

    private:
        std::vector<std::unique_ptr<Thread>> threads_;
    };
}

#endif //TINK_MESSAGE_HANDLER_H
