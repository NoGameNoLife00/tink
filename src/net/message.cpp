//
// Created by admin on 2020/6/1.
//

#include "message.h"

namespace tink {

    uint Message::GetId() const {
        return id_;
    }

    void Message::SetId(uint id) {
        id_ = id;
    }

    uint Message::GetDataLen() const {
        return data_len_;
    }

    void Message::SetDataLen(uint dataLen) {
        data_len_ = dataLen;
    }

    const std::shared_ptr<byte> &Message::GetData() const {
        return data_;
    }

    void Message::SetData(const std::shared_ptr<byte> &data) {
        Message::data_ = data;
    }

}
