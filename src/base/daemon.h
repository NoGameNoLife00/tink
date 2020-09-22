#ifndef TINK_DAEMON_H
#define TINK_DAEMON_H

#include <string>

namespace tink {
    class Daemon {
        static int Init(const std::string& pid_file);
        static int Exit(const std::string& pid_file);
    };
}



#endif //TINK_DAEMON_H