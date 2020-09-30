#include <error_code.h>
#include <message.h>

namespace tink {

    int32_t NetMessage::GetId() const {
        return id;
    }

    void NetMessage::SetId(uint32_t id) {
        this->id = id;
    }

    uint32_t NetMessage::GetDataLen() const {
        return data_len;
    }

    void NetMessage::SetDataLen(uint32_t len) {
        this->data_len = len;
    }

    UBytePtr &NetMessage::GetData() {
        return data;
    }

    void NetMessage::SetData(UBytePtr &data) {
        data = std::move(data);
    }

    int NetMessage::Init(uint32_t id, uint32_t len, UBytePtr &data) {
        this->id = id;
        data_len = len;
        this->data = std::move(data);
        return E_OK;
    }

    int Message::Init(uint32_t source, int32_t session, UBytePtr& data, size_t size) {
        this->source = source;
        this->session = session;
        this->data = std::move(data);
        this->size = size;
        return E_OK;
    }
}
