//
// Created by 陈涛 on 2020/9/15.
//

#include <spdlog/spdlog.h>
#include "timer.h"

namespace tink {
    namespace TimeUtil {
        uint64_t GetTime() {
            uint64_t tm;
            struct timespec ti;
            clock_gettime(CLOCK_MONOTONIC, &ti);
            tm = static_cast<uint64_t>(ti.tv_sec) * 100;
            tm += ti.tv_nsec / 10000000;
            return tm;
        }

        uint64_t GetThreadTime() {
            struct timespec ti;
            clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ti);
            return (uint64_t)ti.tv_sec * MICRO_SEC + (uint64_t)ti.tv_nsec / (NANO_SEC / MICRO_SEC);
        }
    }

    void Timer::Init() {
        for (auto& list : near_) {
            list.clear();
        }
        for (int i = 0; i < TIME_LIST_CNT; i++) {
            for (auto& list : t_[i]) {
                list.clear();
            }
        }
        current_ = 0;
        struct timespec ti;
        clock_gettime(CLOCK_REALTIME, &ti);
        start_time_ = static_cast<uint32_t>(ti.tv_sec);
        current_ = static_cast<uint32_t>(ti.tv_nsec / 10000000);
        current_point_ = static_cast<uint64_t>(ti.tv_sec) * 100 + ti.tv_nsec / 10000000;
    }

    void Timer::UpdateTime() {
        uint64_t tm = TimeUtil::GetTime();
        if (tm < current_point_) {
            spdlog::error("time diff error: change from {} to {}", tm, current_point_);
            current_point_ = tm;
        } else if (tm != current_point_) {
            uint32_t diff = static_cast<uint32_t>(tm - current_point_);
            current_point_ = tm;
            current_ += diff;
            for (int i = 0; i < diff; i++) {
                Update_();
            }
        }
    }

    void Timer::Update_() {
        std::unique_lock<std::mutex> lock(mutex_);
        Execute_();
        Shift_();
    }

    void Timer::Execute_() {
        int idx = time_ & TIME_NEAR_MASK;
        // todo 是否能使用迭代器？？
        for(auto &current : near_[idx]) {
            mutex_.unlock();

            mutex_.lock();
        }
    }

    void Timer::DispatchList(TimerNode &curr) {

    }
}


