//
// Created by admin on 2020/5/26.
//

#ifndef TINK_REQUEST_H
#define TINK_REQUEST_H


#include <irequest.h>
#include <imessage.h>

namespace tink {
    class Request : public IRequest {
    public:
        Request(std::shared_ptr<IConnection>& conn, std::shared_ptr<IMessage>& msg);
        std::shared_ptr<IConnection>& GetConnection();
        std::shared_ptr<byte>& GetData();
        uint32_t GetMsgId();
    private:
        std::shared_ptr<IConnection> conn;
//        std::shared_ptr<byte> data;

        std::shared_ptr<IMessage> msg;
    };

}


#endif //TINK_REQUEST_H
