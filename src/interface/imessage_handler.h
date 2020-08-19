
#ifndef TINK_IMESSAGE_HANDLER_H
#define TINK_IMESSAGE_HANDLER_H

#include "irequest.h"
#include "irouter.h"

namespace tink {
    class IRequest;
    class IRouter;
    typedef std::shared_ptr<IRouter> IRouterPtr;
    typedef std::shared_ptr<IRequest> IRequestPtr;
    class IMessageHandler {
    public:
        // 调度执行Router消息处理方法
        virtual int DoMsgHandle(IRequest &request) = 0;
        // 绑定Message对应的Router
        virtual int AddRouter(uint32_t msg_id, IRouterPtr &router) =0;
        virtual int StartWorkerPool() { return 0; };
        virtual int SendMsgToTaskQueue(IRequestPtr &request) { return 0; };
    };
    typedef std::shared_ptr<IMessageHandler> IMessageHandlerPtr;
}

#endif //TINK_IMESSAGE_HANDLER_H
