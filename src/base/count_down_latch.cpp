#include <count_down_latch.h>
namespace tink {
    CountDownLatch::CountDownLatch(int count)
        : mutex_(), condition_(), count_(count) {
    }

    void CountDownLatch::Wait() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (count_ > 0) {
            condition_.wait(lock);
        }
    }

    void CountDownLatch::CountDown() {
        std::unique_lock<std::mutex> lock(mutex_);
        --count_;
        if (count_ == 0) {
            condition_.notify_all();
        }
    }

    int CountDownLatch::GetCount() const {
        std::lock_guard<std::mutex> guard(mutex_);
        return count_;
    }

}

