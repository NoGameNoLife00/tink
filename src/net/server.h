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
        int Run(); // 运行
        int Stop(); // 停止
        // 给当前服务注册一个路由方法，供客户端链接处理使用
        int AddRouter(uint32_t msg_id, std::shared_ptr<BaseRouter> &router);
        void OperateEvent(uint32_t fd, uint32_t id, int op, int state);
        ConnManagerPtr& GetConnMng() {return conn_mng_;};
        void SetOnConnStart(ConnHookFunc &&func);
        void SetOnConnStop(ConnHookFunc &&func);
        void CallOnConnStart(ConnectionPtr &&conn);
        void CallOnConnStop(ConnectionPtr &&conn);
        SocketServerPtr GetSocketServer() { return socket_server_; }

    private:

        static const int ListenID = 0;
        static const int ConnStartID = 1000;
        static const int InitEvenListCount = 32;
        typedef std::array<struct epoll_event, InitEvenListCount> EventList;
        void HandleAccept_();
        void HandleEvents_(int event_num);
        void DoRead_(int id);
        void DoWrite_(int id);
        void DoError_(int id);
        string name_;
        // server的消息管理模块
        MessageHandlerPtr msg_handler_;
        // server的连接管理器
        ConnManagerPtr conn_mng_;

        int epoll_fd_;
        std::unique_ptr<Socket> listen_socket_;
        SockAddressPtr listen_addr_;
        EventList events_;

        // 连接启动和停止的钩子函数
        ConnHookFunc on_conn_start_;
        ConnHookFunc on_conn_stop_;

        SocketServerPtr socket_server_;
    };
}


#endif