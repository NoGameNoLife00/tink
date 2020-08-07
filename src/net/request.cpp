#include <request.h>
namespace tink {
    Request::Request(std::shared_ptr<IConnection> conn, IMessagePtr &msg) {
        this->conn_ = conn;
        this->msg_ = msg;
    }

    IConnectionPtr & Request::GetConnection() {
        return conn_;
    }

    BytePtr& Request::GetData() {
        return msg_->GetData();
    }

    int32_t Request::GetMsgId() {
        return msg_->GetId();
    }
}

