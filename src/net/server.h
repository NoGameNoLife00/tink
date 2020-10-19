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
#include <config_mng.h>

#define TINK_SERVER tink::Singleton<tink::Server>::GetInstance()

namespace tink {
    class Connection;
    class MessageHandler;
    class ConnManager;
    class BaseRouter;
    typedef std::shared_ptr<Connection> ConnectionPtr;
    typedef std::function<void(ConnectionPtr&)> ConnHookFunc;
    typedef std::shared_ptr<MessageHandler> MessageHandlerPtr;
    typedef std::shared_ptr<ConnManager> ConnManagerPtr;



    class Server : public std::enable_shared_from_this<Server> {
    public:
        int Init(ConfigPtr config);
        int Start(); // 启动
        int Stop(); // 停止
    private:
        ConfigPtr config_;



    };
}


#endif