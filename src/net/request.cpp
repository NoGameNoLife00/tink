#include <request.h>
#include <global_mng.h>

namespace tink {
    Request::Request(IConnectionPtr &conn, IMessagePtr& msg) :conn_(conn) {
        msg_ = std::move(msg);
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

    Request::~Request() {
        logger->debug("request destruction msg_id:%v, conn_id:%v", GetMsgId(), conn_->GetConnId());

    }
}

