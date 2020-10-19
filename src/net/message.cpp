#include <error_code.h>
#include <message.h>

namespace tink {
    int TinkMessage::Init(uint32_t source, int32_t session, UBytePtr& data, size_t size) {
        this->source = source;
        this->session = session;
        this->data = std::move(data);
        this->size = size;
        return E_OK;
    }
}
