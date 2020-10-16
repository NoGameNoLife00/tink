#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <memory>
#include <message_handler.h>
#include <sys/epoll.h>
#include <unordered_map>
#include <map>
#include <vector>
#include <socket.h>
#include <connection.h>
#include <conn_manager.h>
#include <functional>
#include <context.h>
#include <socket_server.h>

//#define TINK_SERVER tink::Singleton<tink::Server>::GetInstance()

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
        int Init(string &name, int ip_version, string &ip, int port);
        int Start(); // 启动
        int Stop(); // 停止
        SocketServerPtr GetSocketServer() { return socket_server_; }
    private:
        static const int ListenID = 0;
        static const int ConnStartID = 1000;
        static const int InitEvenListCount = 32;
        string name_;
        // server的消息管理模块
        MessageHandlerPtr msg_handler_;
        SockAddressPtr listen_addr_;
        EventList events_;
        // 连接启动和停止的钩子函数
        ConnHookFunc on_conn_start_;
        ConnHookFunc on_conn_stop_;
    };
}


#endif