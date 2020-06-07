//
// Created by admin on 2020/6/7.
//

#include <error_code.h>
#include <msg_handler.h>
namespace tink {
    int MsgHandler::DoMsgHandle(IRequest &request) {
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

    int MsgHandler::AddRouter(uint msg_id, std::shared_ptr<IRouter> &router) {
        if (apis.find(msg_id) != apis.end()) {
            printf("msg repeat add, msg_id=%d", msg_id);
            return E_MSG_REPEAT_ROUTER;
        }
        apis[msg_id] = router;
        return E_OK;
    }
}

