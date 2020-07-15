
#include <request.h>
#include "request.h"

namespace tink {
    Request::Request(std::shared_ptr<IConnection> &conn, std::shared_ptr<IMessage> &msg) {
        this->conn = conn;
        this->msg = msg;
    }

    std::shared_ptr<IConnection> & Request::GetConnection() {
        return conn;
    }

    std::shared_ptr<byte>& Request::GetData() {
        return msg->GetData();
    }

    int32_t Request::GetMsgId() {
        return msg->GetId();
    }
}

