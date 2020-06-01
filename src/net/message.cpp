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
        return data_len;
    }

    void Message::SetDataLen(uint dataLen) {
        data_len = dataLen;
    }

    const std::shared_ptr<byte> &Message::GetData() const {
        return data;
    }

    void Message::SetData(const std::shared_ptr<byte> &data) {
        Message::data = data;
    }

}
