#ifndef TINK_MESSAGE_H
#define TINK_MESSAGE_H


#include <imessage.h>
#include <leaked_object_detector.h>

namespace tink {
    class Message : public IMessage{
    public:
        int Init(uint32_t id, uint32_t len, const BytePtr &data);
        int32_t GetId() const;

        void SetId(uint32_t id);

        uint32_t GetDataLen() const;

        void SetDataLen(uint32_t dataLen);

        std::shared_ptr<byte> &GetData();

        void SetData(const BytePtr &data);

    private:
        uint32_t id_; // ��ϢID
        uint32_t data_len_; // ��Ϣ����
        std::shared_ptr<byte> data_; // ��Ϣ����

        LEAK_DETECTOR(Message);
    };
}



#endif //TINK_MESSAGE_H
