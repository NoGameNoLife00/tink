#ifndef TINK_MONITOR_H
#define TINK_MONITOR_H

#include <cstdint>
#include <atomic>
#include <memory>
#include <condition_variable>
#include "common.h"

namespace tink {
    class MonitorNode {
    public:
        std::atomic_int version;
        int check_version;
        uint32_t source;
        uint32_t destination;

        void Trigger(uint32_t _source, uint32_t _destination);
        void Check();

    };
    using MonitorNodePtr = std::shared_ptr<MonitorNode>;

    class Monitor {
    public:
        int count;
        int sleep;
        int quit;
        std::vector<MonitorNodePtr> m;
        mutable Mutex mutex;
        std::condition_variable cond;
    };
    using MonitorPtr = std::shared_ptr<Monitor>;
}


#endif //TINK_MONITOR_H
