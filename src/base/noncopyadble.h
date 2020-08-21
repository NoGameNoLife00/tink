#ifndef TINK_NONCOPYADBLE_H
#define TINK_NONCOPYADBLE_H

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

#endif //TINK_NONCOPYADBLE_H
