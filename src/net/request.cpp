
#include <request.h>
#include "request.h"

namespace tink {
    Request::Request(IConnection &conn, std::shared_ptr<IMessage> &msg) : conn_(conn) {
        this->msg_ = msg;
    }

    IConnection & Request::GetConnection() {
        return conn_;
    }

    std::shared_ptr<byte>& Request::GetData() {
        return msg_->GetData();
    }

    int32_t Request::GetMsgId() {
        return msg_->GetId();
    }
}

