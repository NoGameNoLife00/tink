#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <memory>
#include <sys/epoll.h>
#include <unordered_map>
#include <map>
#include <vector>
#include <functional>

#include "base/context.h"
#include "base/config.h"
#include "base/module_manager.h"
#include "base/handle_manager.h"
#include "net/socket.h"
#include "net/socket_server.h"
#include "net/harbor.h"
#include "net/timer_manager.h"

namespace tink {
    class Server;
    class HandleMgr;
    class ModuleMgr;
    class Harbor;

    typedef std::shared_ptr<Server> ServerPtr;
    ServerPtr& GetGlobalServer();

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

    class Server : public noncopyable, std::enable_shared_from_this<Server> {
    public:
        int Init(ConfigPtr config);
        int Start(); // 启动
        int Stop(); // 停止
        void Bootstrap(const std::string& cmdline);

        ModuleMgr* GetModuleMgr() const;
        HandleMgr* GetHandlerMgr() const;
        Harbor* GetHarbor() const;
        GlobalMQ* GetGlobalMQ() const;
        TimerMgr* GetTimerMgr() const;
        SocketServer* GetSocketServer() const;
    private:
        ConfigPtr config_;

        std::unique_ptr<ModuleMgr> module_mgr_;
        std::unique_ptr<HandleMgr> handler_mgr_;
        std::unique_ptr<Harbor> harbor_;
        std::unique_ptr<GlobalMQ> global_mq_;
        std::unique_ptr<TimerMgr> timer_mgr_;
        std::unique_ptr<SocketServer> socket_server_;

    };
}


#endif