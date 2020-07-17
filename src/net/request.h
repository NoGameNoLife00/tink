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
        Request(IConnection& conn, std::shared_ptr<IMessage>& msg);
        IConnection & GetConnection();
        std::shared_ptr<byte>& GetData();
        int32_t GetMsgId();
    private:
        IConnection& conn_;
        std::shared_ptr<IMessage> msg_;
    };

}


#endif //TINK_REQUEST_H
