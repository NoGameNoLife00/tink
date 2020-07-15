
#include <request.h>
#include "request.h"

namespace tink {
    Request::Request(std::shared_ptr<IConnection> &conn, std::shared_ptr<IMessage> &msg) {

#include <request.h>
#include "request.h"

namespace tink {
    Request::Request(std::shared_ptr<IConnection> &conn, std::shared_ptr<IMessage> &msg) {
        this->conn = conn;
        this->msg = msg;
        this->conn_ = conn;
        this->msg_ = msg;
    }

    std::shared_ptr<IConnection> & Request::GetConnection() {
        return conn_;
    }

    std::shared_ptr<byte>& Request::GetData() {
        return msg_->GetData();
    }

    int32_t Request::GetMsgId() {
        return msg_->GetId();
    }
}

