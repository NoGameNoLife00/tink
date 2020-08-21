#include <error_code.h>
#include <message_handler.h>
#include <global_mng.h>
#include <cstring>
#include <thread.h>
#include <assert.h>

namespace tink {
    int MessageHandler::DoMsgHandle(IRequest &request) {
        // 根据MsgId 调度对应router业务
        auto iter = apis.find(request.GetMsgId());
        if (iter != apis.end()) {
            iter->second->PreHandle(request);
            iter->second->Handle(request);
            iter->second->PostHandle(request);
        } else {
            logger->info("handler msg_id =%v not find", request.GetMsgId());
        }
        return E_OK;
    }

    int MessageHandler::AddRouter(uint32_t msg_id, IRouterPtr &router) {
        if (apis.find(msg_id) != apis.end()) {
            logger->info("msg repeat add, msg_id=%v", msg_id);
            return E_MSG_REPEAT_ROUTER;
        }
        apis[msg_id] = router;
        return E_OK;
    }

    int MessageHandler::Init() {
        worker_pool_size = GlobalInstance->GetWorkerPoolSize();
        return 0;
    }

    int MessageHandler::StartWorkerPool() {
        assert(threads_.empty());
        threads_.reserve(worker_pool_size);
        for (int i = 0; i < worker_pool_size; i++) {
            // 创建一个消息队列和启动worker线程
            task_queue.push_back(std::make_shared<IRequestMsgQueue>());
            threads_.emplace_back(std::make_unique<Thread>(std::bind(StartOneWorker, std::ref(*this),  i), "worker"+i+1));
            threads_[i]->Start();
        }
        return 0;
    }

    void MessageHandler::StartOneWorker(MessageHandler& handler, int worker_id) {
//        WorkerInfo* info = static_cast<WorkerInfo*>(worker_info_ptr);
//        if (!info) {
//            logger->info("worker thread run error, info_ptr is null\n");
//            return nullptr;
//        }
//        MessageHandler *handler = info->msg_handler;
//        int worker_id = info->worker_id;
        IRequestMsgQueuePtr msg_queue = handler.task_queue[worker_id];
        logger->info("work id = %v, is started...\n", worker_id);
        while (true) {
            IRequestPtr req;
            // 消息队列取出router
            msg_queue->Pop(req, true);
            if (req->GetMsgId() == MSG_ID_EXIT)
                break;
            handler.DoMsgHandle(*req);
            logger->debug("req msg %v", req->GetMsgId());
        }
        logger->info("work id = %v, is stopped...\n", worker_id);
    }

    int MessageHandler::SendMsgToTaskQueue(IRequestPtr &request) {
        // 平均分配给TaskQueue (暂时按客户端connId分配
        int conn_id = request->GetConnection()->GetConnId();
        int workerId = conn_id % worker_pool_size;
        logger->info("add conn_id = %v, request msg_id =%v to worker_id=%v", conn_id, request->GetMsgId(), workerId);
        task_queue[workerId]->Push(request);
        return 0;
    }
}

