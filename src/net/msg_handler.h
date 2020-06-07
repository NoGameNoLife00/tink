//
// Created by admin on 2020/6/7.
//

#ifndef TINK_MSG_HANDLER_H
#define TINK_MSG_HANDLER_H


#include <map>
#include <irouter.h>
#include <imsg_handler.h>

namespace tink {
    class MsgHandler : public IMsgHandler {
    public:
        int DoMsgHandle(IRequest &request) override;

        int AddRouter(uint msg_id, std::shared_ptr<IRouter> &router) override;

    private:
        // 存放每个MsgId对应的方法
        std::map<uint, std::shared_ptr<IRouter>> apis;
    };
}



#endif //TINK_MSG_HANDLER_H
