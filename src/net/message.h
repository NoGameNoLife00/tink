//
// Created by admin on 2020/6/1.
//

#ifndef TINK_MESSAGE_H
#define TINK_MESSAGE_H


#include <imessage.h>

namespace tink {
    class Message : public IMessage{
    public:
        int Init(uint id, uint len, const std::shared_ptr<byte> &data);
        uint GetId() const;

        void SetId(uint id);

        uint GetDataLen() const;

        void SetDataLen(uint dataLen);

        std::shared_ptr<byte> &GetData();

        void SetData(const std::shared_ptr<byte> &data);

    private:
        uint id_; // 消息ID
        uint data_len_; // 消息长度
        std::shared_ptr<byte> data_; // 消息数据
    };
}



#endif //TINK_MESSAGE_H
