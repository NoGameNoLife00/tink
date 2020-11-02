#include <error_code.h>
#include <message.h>

namespace tink {
    int TinkMessage::Init(uint32_t _source, int32_t _session, BytePtr &_data, size_t _size) {
        source = _source;
        session = _session;
        data = std::move(_data);
        size = _size;
        return E_OK;
    }
}
