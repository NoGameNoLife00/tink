#ifndef TINK_DAEMON_H
#define TINK_DAEMON_H

#include <string>

namespace tink {
    namespace Daemon {
        int Init(const std::string& pid_file);
        int Exit(const std::string& pid_file);
    };
}



#endif //TINK_DAEMON_H
