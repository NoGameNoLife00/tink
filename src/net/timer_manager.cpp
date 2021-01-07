#include "spdlog/spdlog.h"
#include "base/handle_manager.h"
#include "net/timer_manager.h"
#include "net/message.h"
#include "error_code.h"
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
            struct timespec ti{};
            clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ti);
            return (uint64_t)ti.tv_sec * MICRO_SEC + (uint64_t)ti.tv_nsec / (NANO_SEC / MICRO_SEC);
        }

        void SysTime(uint32_t &sec, uint32_t &cs) {
            struct timespec ti{};
            clock_gettime(CLOCK_REALTIME, &ti);
            sec = (uint32_t)ti.tv_sec;
            cs = (uint32_t)(ti.tv_nsec / 10000000);
        }
    }

    void TimerMgr::Init() {
        for (auto& list : near_) {
            list.clear();
        }
        for (auto & i : t_) {
            for (auto& list : i) {
                list.clear();
            }
        }
        uint32_t  current;
        TimeUtil::SysTime(start_time_, current);
        current_ = current;
        current_point_ = TimeUtil::GetTime();
    }

    void TimerMgr::UpdateTime() {
        uint64_t tm = TimeUtil::GetTime();
        if (tm < current_point_) {
            spdlog::error("time diff error: change from {} to {}", tm, current_point_);
            current_point_ = tm;
        } else if (tm != current_point_) {
            auto diff = static_cast<uint32_t>(tm - current_point_);
            current_point_ = tm;
            current_ += diff;
            for (int i = 0; i < diff; i++) {
                Update_();
            }
        }
    }

    void TimerMgr::Update_() {
        std::unique_lock<std::mutex> lock(mutex_);
        Execute_();
        Shift_();
        Execute_();
    }

    void TimerMgr::Execute_() {
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

    void TimerMgr::DispatchList_(TimerNodeList &curr) {
        for (auto& node : curr) {
            TimerEvent& event = node->event;
            TinkMessage msg;
            msg.source = 0;
            msg.session = event.session;
            msg.data = nullptr;
            msg.size = static_cast<size_t>(PTYPE_RESPONSE) << MESSAGE_TYPE_SHIFT;
            server_->GetHandlerMgr()->PushMessage(event.handle, msg);
        }
    }

    void TimerMgr::AddNode_(TimerNodePtr &node) {
        uint32_t tm = node->expire;
        if ((tm | TIME_NEAR_MASK) != (time_ | TIME_NEAR_MASK)) {
            uint32_t mask = TIME_NEAR << TIME_NEAR_SHIFT;
            int i;
            for (i = 0; i < 3; i++) {
                if ((tm | (mask - 1) == (time_ | (mask - 1)))) {
                    break;
                }
                mask <<= TIME_NEAR_SHIFT;
            }
            t_[i][(tm >> (TIME_NEAR_SHIFT + i * TIME_LEVEL_SHIFT)) & TIME_LEVEL_MASK].emplace_back(std::move(node));
        } else {
            near_[tm & TIME_NEAR_MASK].emplace_back(std::move(node));
        }
    }

    void TimerMgr::Add_(int time, const TimerEvent& event) {
        TimerNodePtr node = std::make_unique<TimerNode>();
        memcpy(&node->event, &event, sizeof(TimerEvent));

        std::unique_lock<std::mutex> lock(mutex_);
        node->expire = time + time_;
        AddNode_(node);
    }

    void TimerMgr::Shift_() {
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

    void TimerMgr::MoveList_(int level, int idx) {
        for (auto& current : t_[level][idx]) {
            AddNode_(current);
        }
        t_[level][idx].clear();
    }

    int TimerMgr::TimeOut(uint32_t handle, int time, int session) {
        if (time <= 0) {
            TinkMessage msg;
            msg.source = 0;
            msg.session = session;
            msg.data = nullptr;
            msg.size = static_cast<size_t>(PTYPE_RESPONSE) << MESSAGE_TYPE_SHIFT;
            if (server_->GetHandlerMgr()->PushMessage(handle, msg)) {
                return E_FAILED;
            }
        } else {
            TimerEvent event;
            event.handle = handle;
            event.session = session;
            Add_(time, event);
        }
        return session;
    }


}


