//
// Created by admin on 2020/6/1.
//

#include <error_code.h>
#include "message.h"

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

    std::shared_ptr<byte> &Message::GetData() {
        return data_;
    }

    void Message::SetData(const std::shared_ptr<byte> &data) {
        Message::data_ = data;
    }

    int Message::Init(uint32_t id, uint32_t len, const std::shared_ptr<byte> &data) {
        id_ = id;
        data_len_ = len;
        data_ = data;
        return E_OK;
    }

}
