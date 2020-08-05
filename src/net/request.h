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
        Request(IConnectionPtr&& conn, IMessagePtr& msg);
        IConnectionPtr & GetConnection();
        BytePtr& GetData();
        int32_t GetMsgId();
    private:
        IConnectionPtr conn_;
        std::shared_ptr<IMessage> msg_;
    };

}


#endif //TINK_REQUEST_H
