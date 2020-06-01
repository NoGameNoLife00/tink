//
// Created by admin on 2020/6/1.
//

#ifndef TINK_MESSAGE_H
#define TINK_MESSAGE_H


#include <imessage.h>

namespace tink {
    class Message : public IMessage{
    public:
        uint GetId() const;

        void SetId(uint id);

        uint GetDataLen() const;

        void SetDataLen(uint dataLen);

        const std::shared_ptr<byte> &GetData() const;

        void SetData(const std::shared_ptr<byte> &data);

    private:
        uint id_; // ��ϢID
        uint data_len; // ��Ϣ����
        std::shared_ptr<byte> data; // ��Ϣ����
    };
}



#endif //TINK_MESSAGE_H
