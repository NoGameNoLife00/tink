#include <error_code.h>
#include <message.h>
#include <global_mng.h>

namespace tink {

    int32_t Message::GetId() const {
        return id_;
    }

    void Message::SetId(uint32_t id) {
        id_ = id;
    }

    uint32_t Message::GetDataLen() const {
        return data_len_;
    }

    void Message::SetDataLen(uint32_t dataLen) {
        data_len_ = dataLen;
    }

    BytePtr &Message::GetData() {
        return data_;
    }

    void Message::SetData(BytePtr &data) {
        data_ = std::move(data);
    }

    int Message::Init(uint32_t id, uint32_t len, BytePtr &data) {
        id_ = id;
        data_len_ = len;
        data_ = std::move(data);
        return E_OK;
    }

    Message::~Message() {
        logger->debug("message destruction id:%v", id_);
    }

}
