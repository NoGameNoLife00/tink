//
// Created by admin on 2020/6/7.
//

#ifndef TINK_IMSG_HANDLER_H
#define TINK_IMSG_HANDLER_H

#include "irequest.h"
#include "irouter.h"

namespace tink {
    class IMsgHandler {
    public:
        // 调度执行Router消息处理方法
        virtual int DoMsgHandle(IRequest &request) {};
        // 绑定Message对应的Router
        virtual int AddRouter(uint msg_id, std::shared_ptr<IRouter> &router) {};
        virtual ~IMsgHandler() {};
    };
}


#endif //TINK_IMSG_HANDLER_H
