//
// Created by ���� on 2020/8/24.
//

#ifndef TINK_BUFFER_H
#define TINK_BUFFER_H


#include <cstdint>
#include <type.h>
#include <cstring>
#include <error_code.h>
#include <noncopyadble.h>

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
        int Capacity() const {return sizeof data_; }
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
}

#endif //TINK_BUFFER_H