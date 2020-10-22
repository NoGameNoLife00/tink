#include <handle_storage.h>
#include <spdlog/spdlog.h>
#include "monitor.h"

namespace tink {
    void MonitorNode::Trigger(uint32_t source, uint32_t destination) {
        this->source = source;
        this->destination = destination;
        ++version;
    }

    void MonitorNode::Check() {
        if (version == check_version) {
            if (destination) {
                HANDLE_STORAGE.ContextEndless(destination);
                spdlog::error("A message from [ :%08x ] to [ :%08x ] maybe in an endless loop (version = %d)",
                              source, destination, version);
            }
        } else {
            check_version = version;
        }
    }

}
