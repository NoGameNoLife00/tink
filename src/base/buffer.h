#ifndef TINK_BUFFER_H
#define TINK_BUFFER_H


#include <cstdint>
#include <common.h>
#include <cstring>
#include <error_code.h>
#include <noncopyable.h>

namespace tink {
    class FixBuffer : noncopyable {
    public:
        FixBuffer(size_t s) : size(s), len_(0) {
            data_ = new byte[size];
        }

        ~FixBuffer() {
            delete [] data_;
        }
        int Append(const char* buf, size_t len) {
            if (Remain() > len) {
                memcpy(data_+len_, buf, len);
                len_ += len;
                return E_OK;
            }
            return E_BUFF_OVERSIZE;
        }

        const byte * Data() const { return data_; }
        int Length() const {return static_cast<int>(len_); }
        int Capacity() const {return size; }
        int Remain() const { return size - len_; }

        void Add(size_t len) { len_ += len; }
        void Reset() { len_=0; }
        void BZero() { memset(data_, 0, sizeof data_); }

        std::string ToString() const { return std::string(data_, len_); }

    private:
        size_t size;
        byte *data_;
        size_t len_;
    };
    typedef std::unique_ptr<FixBuffer> FixBufferPtr;

    class DynamicBuffer {
    public:
        explicit DynamicBuffer(DataPtr &buff, size_t sz = 0, int offset = 0) :
                buffer_(buff), size_(sz), offset_(offset) {}
        explicit DynamicBuffer(size_t sz) : buffer_(new byte[sz]), size_(sz), offset_(0) {
        }
        DataPtr& GetData() {
            return buffer_;
        }
        int GetOffset() const { return offset_; }
        int GetSize() const { return size_; }
        void SetOffset(int n) { offset_ = n; }

    private:
        DataPtr buffer_;
        size_t  size_;
        int offset_;
    } ;

    typedef std::shared_ptr<DynamicBuffer> DataBufferPtr;

    constexpr int SOCKET_BUFFER_MEMORY = 0;
    constexpr int SOCKET_BUFFER_OBJECT = 1;
    constexpr int SOCKET_BUFFER_RAWPOINTER = 2;
    typedef struct SocketSendBuffer_ {
        int id;
        int type;
        DataPtr buffer;
        size_t sz;
        void Init(int id, DataPtr buffer, int sz) {
            this->id = id;
            this->buffer = buffer;
            this->type = sz < 0 ? SOCKET_BUFFER_OBJECT : SOCKET_BUFFER_MEMORY;
            this->sz = sz;
        }
        void FreeBuffer() {
            buffer.reset();
        }
    }SocketSendBuffer;


}

#endif //TINK_BUFFER_H
