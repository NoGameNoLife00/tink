
#ifndef TINK_IMESSAGE_HANDLER_H
#define TINK_IMESSAGE_HANDLER_H

#include "irequest.h"
#include "irouter.h"

namespace tink {
    class IMessageHandler {
    public:
        // 调度执行Router消息处理方法
        virtual int DoMsgHandle(IRequest &request) = 0;
        // 绑定Message对应的Router
        virtual int AddRouter(uint32_t msg_id, std::shared_ptr<IRouter> &router) =0;
//        virtual ~IMessageHandler() {};
    };
}

#endif //TINK_IMESSAGE_HANDLER_H
