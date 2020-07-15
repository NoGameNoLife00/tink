//
// Created by admin on 2020/6/7.
//

#include <error_code.h>
#include <message_handler.h>
#include <global_mng.h>

namespace tink {
    int MessageHandler::DoMsgHandle(IRequest &request) {
        // 根据MsgId 调度对应router业务
        auto iter = apis.find(request.GetMsgId());
        if (iter != apis.end()) {
            iter->second->PreHandle(request);
            iter->second->Handle(request);
            iter->second->PostHandle(request);
        } else {
            printf("handler msg_id =%d not find", request.GetMsgId());
        }
        return E_OK;
    }

    int MessageHandler::AddRouter(uint32_t msg_id, IRouterPtr &router) {
        if (apis.find(msg_id) != apis.end()) {
            printf("msg repeat add, msg_id=%d", msg_id);
            return E_MSG_REPEAT_ROUTER;
        }
        apis[msg_id] = router;
        return E_OK;
    }

    int MessageHandler::Init() {
        std::shared_ptr<GlobalMng> globalObj = tink::Singleton<tink::GlobalMng>::GetInstance();
        worker_pool_size = globalObj->GetWorkerPoolSize();
        return 0;
    }

    int MessageHandler::StartWorkerPool() {
        for (int i = 0; i < worker_pool_size; i++) {
            // 创建一个消息队列和启动worker线程
            task_queue.push_back(std::make_shared<IRequestMsgQueue>());

        }

        return 0;
    }

    MessageHandler::~MessageHandler() {

    }

    int MessageHandler::StartOneWorker(int worker_id, IRequestMsgQueuePtr &msg_queue) {
        printf("work id = %d, is started...\n", worker_id);
        while (true) {
            IRequestPtr req;
            // 消息队列取出router
            msg_queue->Pop(req, true);
            if (req->GetMsgId() == MSG_ID_EXIT)
            DoMsgHandle(*req);
        }
        return 0;
    }

    int MessageHandler::SendMsgToTaskQueue(IRequestPtr &request) {
        // 平均分配给TaskQueue (暂时按客户端connId分配
        int conn_id = request->GetConnection()->GetConnId();
        int workerId = conn_id % worker_pool_size;
        printf("add conn_id = %d, request msg_id =%d", conn_id, request->GetMsgId());
        task_queue[workerId]->Push(request);
        return 0;
    }
}

