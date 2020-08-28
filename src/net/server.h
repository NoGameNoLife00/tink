#ifndef SERVER_H
#define SERVER_H

#include <iserver.h>
#include <string>
#include <memory>
#include <imessage_handler.h>
#include <sys/epoll.h>
#include <unordered_map>
#include <map>
#include <iconn_manager.h>
#include <vector>
#include <socket.h>

namespace tink {

    class Server : public IServer
    , public std::enable_shared_from_this<Server>
            {
    public:
        int Init(string &name, int ip_version,
                 string &ip, int port,
                 IMessageHandlerPtr &&msg_handler);
        int Start();
        int Run();
        int Stop();
        int AddRouter(uint32_t msg_id, std::shared_ptr<IRouter> &router);
        void OperateEvent(uint32_t fd, uint32_t id, int op, int state);
        IConnManagerPtr& GetConnMng() {return conn_mng_;};
        void SetOnConnStart(ConnHookFunc &&func);
        void SetOnConnStop(ConnHookFunc &&func);
        void CallOnConnStart(IConnectionPtr &&conn);
        void CallOnConnStop(IConnectionPtr &&conn);
    private:
        void HandleAccept_();
        void HandleEvents_(int event_num);
        void DoRead_(int id);
        void DoWrite_(int id);
        void DoError_(int id);
        string name_;
        // server的消息管理模块
        IMessageHandlerPtr msg_handler_;
        // server的连接管理器
        IConnManagerPtr conn_mng_;

        static const int ListenID = 0;
        static const int ConnStartID = 1000;
        static const int InitEvenListCount = 32;
        typedef std::array<struct epoll_event, InitEvenListCount> EventList;
        int epoll_fd_;
//        int listen_fd_;
        std::unique_ptr<Socket> listen_socket_;
        SockAddressPtr listen_addr_;
        EventList events_;

        // 连接启动和停止的钩子函数
        ConnHookFunc on_conn_start_;
        ConnHookFunc on_conn_stop_;
    };
}


#endif