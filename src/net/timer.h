#ifndef TINK_TIMER_H
#define TINK_TIMER_H

#include <cstdint>
#include <list>
#include <shared_mutex>

#define TIMER tink::Singleton<tink::Timer>::GetInstance()

namespace tink {
    // 定时器事件
    typedef struct TimerEvent_ {
        uint32_t handle;
        int32_t session;
    } TimerEvent;

    // 定时器节点
    typedef struct TimerNode_ {
        uint32_t expire; // 到期滴答数
        TimerEvent event;
    } TimerNode;

    typedef std::unique_ptr<TimerNode> TimerNodePtr;
    namespace TimeUtil {
        constexpr static int NANO_SEC = 1000000000;
        constexpr static int MICRO_SEC = 1000000;
        uint64_t GetTime();
        uint64_t GetThreadTime();
        void SysTime(uint32_t &sec, uint32_t &cs);
    }

    class Timer {
    public:
        void Init();
        void UpdateTime();
        int TimeOut(uint32_t handle, int time, int session);

        uint64_t Now() const { return current_; }
        uint64_t StartTime() const { return start_time_; }
    private:
        typedef std::list<TimerNodePtr> TimerNodeList;
        void Add_(int time, const TimerEvent& event);
        void AddNode_(TimerNodePtr& node);
        void Update_();
        void Execute_();
        void Shift_();
        void MoveList_(int level, int idx);
        static void DispatchList_(TimerNodeList &curr);
        constexpr static int TIME_NEAR_SHIFT = 8;
        constexpr static int TIME_LEVEL_SHIFT = 6;
        constexpr static int TIME_NEAR = 1 << TIME_NEAR_SHIFT;
        constexpr static int TIME_LEVEL = 1 << TIME_NEAR_SHIFT;
        constexpr static int TIME_LIST_CNT = 4;
        constexpr static int TIME_NEAR_MASK = TIME_NEAR - 1;
        constexpr static int TIME_LEVEL_MASK = TIME_LEVEL - 1;
        uint64_t current_;
        uint64_t current_point_;
        uint32_t time_; // 启动到现在走过的滴答数
        uint32_t start_time_;

        TimerNodeList near_[TIME_NEAR];
        TimerNodeList t_[TIME_LIST_CNT][TIME_LEVEL];
        mutable std::mutex mutex_;
    };
}




#endif //TINK_TIMER_H
