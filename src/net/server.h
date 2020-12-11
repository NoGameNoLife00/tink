#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <memory>
#include <sys/epoll.h>
#include <unordered_map>
#include <map>
#include <vector>
#include <socket.h>
#include <functional>
#include <context.h>
#include <socket_server.h>
#include <config.h>

#define TINK_SERVER tink::Singleton<tink::Server>::GetInstance()

namespace tink {
    namespace Global {
        extern thread_local uint32_t t_handle;
        extern uint32_t monitor_exit;
        extern bool profile;

        inline uint32_t MonitorExit() {return monitor_exit; }
        inline void SetMonitorExit(uint32_t m) { monitor_exit = m; }
        inline bool Profile() { return profile; }
        inline void SetProfile(bool active) { profile = active; }
        inline uint32_t CurrentHandle() { return t_handle; }
        inline void InitThread(int m) { t_handle = static_cast<uint32_t>(-m); }
        inline void SetHandle(uint32_t h) { t_handle = h; }
    }

    class Server : public std::enable_shared_from_this<Server> {
    public:
        int Init(ConfigPtr config);
        int Start(); // 启动
        int Stop(); // 停止
        void Bootstrap(const std::string& cmdline);
    private:
        ConfigPtr config_;
    };
}


#endif