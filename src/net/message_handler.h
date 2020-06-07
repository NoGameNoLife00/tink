
#ifndef TINK_MESSAGE_HANDLER_H
#define TINK_MESSAGE_HANDLER_H


#include <map>
#include <irouter.h>
#include <imessage_handler.h>

namespace tink {
    class MessageHandler : public IMessageHandler {
    public:
        MessageHandler();
        // 存放每个MsgId对应的方法
        std::map<uint, std::shared_ptr<IRouter>> apis;
        // 调度执行Router消息处理方法
        virtual int DoMsgHandle(IRequest &request);
        // 绑定Message对应的Router
        virtual int AddRouter(uint msg_id, std::shared_ptr<IRouter> &router);

    };
}

#endif //TINK_MESSAGE_HANDLER_H
