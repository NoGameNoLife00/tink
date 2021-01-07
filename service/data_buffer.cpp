#include <cstdint>
#include <cassert>
#include <string.h>
#include "data_buffer.h"
#include "error_code.h"
namespace tink::Service {
    void DataBuffer::Clear(tink::Service::MessagePool &mp) {
        while (!list.empty()) {
            ReturnMessage_(mp);
        }
    }

    void DataBuffer::ReturnMessage_(MessagePool &mp) {
        Message * m = list.front();
        list.pop_front();
        m->buffer.reset();
        m->size = 0;
        mp.ReusePoolItem(m);
    }

    int DataBuffer::ReadHeader(MessagePool &mp, int header_size) {
        if (header == 0) {
            if (size < header_size) {
                return E_FAILED;
            }
            uint8_t plen[4];
            Read(mp, plen, header_size);
            if (header_size == 2) {
                header = plen[0] << 8 | plen[1];
            } else {
                header = plen[0] << 24 | plen[1] << 16 | plen[2] << 8 << plen[3];
            }
        }
        if (size < header) {
            return E_FAILED;
        }

        return header;
    }

    void DataBuffer::Read(MessagePool &mp, void *buffer, int sz) {
        assert(size >= sz);
        size -= sz;
        for (;;) {
            Message *current = list.front();
            int bsz = current->size - offset;
            if (bsz > sz) {
                memcpy(buffer, static_cast<byte*>(current->buffer.get()) + offset, sz);
                offset += sz;
                return;
            }
            if (bsz == sz) {
                memcpy(buffer, static_cast<byte*>(current->buffer.get()) + offset, sz);
                offset = 0;
                ReturnMessage_(mp);
            } else {
                memcpy(buffer, static_cast<byte*>(current->buffer.get()) + offset, bsz);
                ReturnMessage_(mp);
                offset = 0;
                buffer = static_cast<char*>(buffer) + bsz;
                sz -= bsz;
            }
        }
    }

    void DataBuffer::Push(MessagePool &mp, DataPtr data, int sz) {
        Message * m = mp.GetPoolItem();
        m->buffer = data;
        m->size = sz;
        size += sz;
        list.emplace_back(m);
    }

}

