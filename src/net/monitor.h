#ifndef TINK_MONITOR_H
#define TINK_MONITOR_H

#include <cstdint>
#include <atomic>
#include <memory>
#include <common.h>
#include <condition_variable>

namespace tink {
    class MonitorNode {
    public:
        std::atomic_int version;
        int check_version;
        uint32_t source;
        uint32_t destination;

        void Trigger(uint32_t source, uint32_t destination);
        void Check();

    };

    typedef std::shared_ptr<MonitorNode> MonitorNodePtr;
    class Monitor {
    public:
        int count;
        int sleep;
        int quit;
        std::vector<MonitorNodePtr> m;
        mutable Mutex mutex;
        std::condition_variable cond;
    };
    typedef std::shared_ptr<Monitor> MonitorPtr;
}


#endif //TINK_MONITOR_H
