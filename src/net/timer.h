#ifndef TINK_TIMER_H
#define TINK_TIMER_H

#include <cstdint>
#include <list>
#include <shared_mutex>

namespace tink {
    typedef struct TimerNode_ {
        uint32_t expire;
    } TimerNode;

    namespace TimeUtil {
        constexpr static int NANO_SEC = 1000000000;
        constexpr static int MICRO_SEC = 1000000;
        uint64_t GetTime();
        uint64_t GetThreadTime();
    }

    class Timer {

        void Init();
        void UpdateTime();
    private:
        void Update_();
        void Execute_();
        void Shift_();
        void DispatchList(TimerNode& curr);
        constexpr static int TIME_NEAR_SHIFT = 8;
        constexpr static int TIME_NEAR = 1 << TIME_NEAR_SHIFT;
        constexpr static int TIME_LEVEL = 1 << TIME_NEAR_SHIFT;
        constexpr static int TIME_LIST_CNT = 4;
        constexpr static int TIME_NEAR_MASK = TIME_NEAR - 1;
        uint64_t current_;
        uint64_t current_point_;
        uint32_t time_;
        uint32_t start_time_;
        typedef std::list<TimerNode> TimerNodeList;
        TimerNodeList near_[TIME_NEAR];
        TimerNodeList t_[TIME_LIST_CNT][TIME_LEVEL];
        mutable std::mutex mutex_;
    };
}




#endif //TINK_TIMER_H
