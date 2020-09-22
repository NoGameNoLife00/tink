//
// Created by ���� on 2020/9/15.
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
        if (!near_[idx].empty()) {
            TimerNodeList temp_list;
            for (auto& node : near_[idx]) {
                temp_list.emplace_back(std::move(node));
            }
            near_->clear();
            mutex_.unlock();
            DispatchList_(temp_list);
            mutex_.lock();
        }
    }

    void Timer::DispatchList_(TimerNodeList &curr) {
        for (auto& node : curr) {
            TimerEvent& event = node->event;
        }
    }

    void Timer::AddNode_(TimerNodePtr &node) {
        uint32_t tm = node->expire;
        if ((tm|TIME_NEAR_MASK) == (time_|TIME_NEAR_MASK)) {
            near_[tm&TIME_NEAR_MASK].emplace_back(std::move(node));
        } else {
            uint32_t mask = TIME_NEAR << TIME_NEAR_SHIFT;
            int i;
            for (i = 0; i < 3; i++) {
                if ((tm|(mask-1) == (time_|(mask-1)))) {
                    break;
                }
                mask <<= TIME_NEAR_SHIFT;
            }
            t_[i][(tm>>(TIME_NEAR_SHIFT + i * TIME_LEVEL_SHIFT)) & TIME_LEVEL_MASK].emplace_back(std::move(node));
        }
    }

    void Timer::Add_(int time, const TimerEvent& event) {
        TimerNodePtr node = std::make_unique<TimerNode>();
        memcpy(&node->event, &event, sizeof(TimerEvent));

        std::unique_lock<std::mutex> lock(mutex_);
        node->expire = time + time_;
        AddNode_(node);
    }

    void Timer::Shift_() {
        int mask = TIME_NEAR;
        uint32_t ct = ++time_;
        if (ct == 0) {
            MoveList_(3, 0);
        } else {
            uint32_t time = ct >> TIME_NEAR_SHIFT;
            int i = 0;
            while ((ct&(mask-1)) == 0) {
                int idx = time & TIME_LEVEL_MASK;
                if (idx != 0) {
                    MoveList_(i, idx);
                    break;
                }
                mask <<= TIME_LEVEL_SHIFT;
                time >>= TIME_LEVEL_SHIFT;
                ++i;
            }
        }
    }

    void Timer::MoveList_(int level, int idx) {
        for (auto& current : t_[level][idx]) {
            AddNode_(current);
        }
        t_[level][idx].clear();
    }
}

