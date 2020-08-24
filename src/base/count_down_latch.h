#ifndef TINK_COUNT_DOWN_LATCH_H
#define TINK_COUNT_DOWN_LATCH_H

#include <type.h>
#include <condition_variable>
#include <noncopyadble.h>

namespace tink {
    class CountDownLatch : noncopyable {
    public:
        explicit CountDownLatch(int count);
        void Wait();
        void CountDown();
        int GetCount() const;

    private:
        mutable Mutex mutex_;
        std::condition_variable condition_;
        int count_;
    };
}




#endif //TINK_COUNT_DOWN_LATCH_H
