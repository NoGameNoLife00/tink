#ifndef TINK_REQUEST_H
#define TINK_REQUEST_H


#include <irequest.h>
#include <imessage.h>

namespace tink {
    class Request : public IRequest {
    public:
        Request(std::shared_ptr<IConnection> conn, IMessagePtr& msg);
        IConnectionPtr & GetConnection();
        BytePtr& GetData();
        int32_t GetMsgId();
    private:
        IConnectionPtr conn_;
        IMessagePtr msg_;
    };

}


#endif //TINK_REQUEST_H
