#ifndef TINK_MESSAGE_H
#define TINK_MESSAGE_H

#include <leaked_object_detector.h>
#include <type.h>
namespace tink {
    class Message {
    public:
        int Init(uint32_t id, uint32_t len, BytePtr &data);
        int32_t GetId() const;

        void SetId(uint32_t id);

        uint32_t GetDataLen() const;

        void SetDataLen(uint32_t dataLen);

        BytePtr &GetData();

        void SetData(BytePtr &data);
        ~Message();
    private:
        uint32_t id_; // ��ϢID
        uint32_t data_len_; // ��Ϣ����
        BytePtr data_; // ��Ϣ����

        LEAK_DETECTOR(Message);
    };

}



#endif //TINK_MESSAGE_H
