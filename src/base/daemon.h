#ifndef TINK_DAEMON_H
#define TINK_DAEMON_H

#include <string>

namespace tink {
    namespace Daemon {
        int Init(std::string_view pid_file);
        int Exit(std::string_view pid_file);
    };
}



#endif //TINK_DAEMON_H
