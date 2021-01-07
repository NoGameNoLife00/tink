#include "base/handle_manager.h"
#include "spdlog/spdlog.h"
#include "net/monitor.h"

namespace tink {
    void MonitorNode::Trigger(uint32_t _source, uint32_t _destination) {
        source = _source;
        destination = _destination;
        ++version;
    }

    void MonitorNode::Check() {
        if (version == check_version) {
            if (destination) {
                GetGlobalServer()->GetHandlerMgr()->ContextEndless(destination);
                spdlog::error("A message from [ {:%08x} ] to [ {:08x} ] maybe in an endless loop (version = {})",
                              source, destination, version);
            }
        } else {
            check_version = version;
        }
    }

}
