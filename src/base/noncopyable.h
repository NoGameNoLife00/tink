#ifndef TINK_NONCOPYABLE_H
#define TINK_NONCOPYABLE_H

namespace tink {
    class noncopyable
    {
    public:
        noncopyable(const noncopyable&) = delete;
        void operator=(const noncopyable&) = delete;

    protected:
        noncopyable() = default;
        ~noncopyable() = default;
    };
}

#endif //TINK_NONCOPYABLE_H
