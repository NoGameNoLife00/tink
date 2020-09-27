#include <request.h>
#include <config_mng.h>

namespace tink {
    Request::Request(ConnectionPtr &conn, MessagePtr& msg) :conn_(conn) {
        msg_ = std::move(msg);
    }

    ConnectionPtr & Request::GetConnection() {
        return conn_;
    }

    BytePtr& Request::GetData() {
        return msg_->GetData();
    }

    int32_t Request::GetMsgId() {
        return msg_->GetId();
    }

    Request::~Request() {
        spdlog::debug("request destruction msg_id:{}, conn_id:{}", GetMsgId(), conn_->GetConnId());

    }

    uint32_t Request::GetDataLen() {
        return msg_->GetDataLen();
    }
}

