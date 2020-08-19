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

namespace tink {

    class Server : public IServer 
    , public std::enable_shared_from_this<Server>
            {
    public:
        int Init(StringPtr &name, int ip_version,
                 StringPtr &ip, int port,
                 IMessageHandlerPtr &&msg_handler);
        int Start();
        int Run();
        int Stop();
        int AddRouter(uint32_t msg_id, std::shared_ptr<IRouter> &router);
        void OperateEvent(uint32_t fd, uint32_t id, int op, int state);
        IConnManagerPtr& GetConnMng() {return conn_mng_;};
    private:
        void HandleAccept_(int listen_fd);
        void HandleEvents_(struct epoll_event *events, int event_num);
        void DoRead_(int fd);
        void DoWrite_(int fd);

        StringPtr name_;
        StringPtr ip_;
        int ip_version_;
        int port_;
        // server的消息管理模块
        IMessageHandlerPtr msg_handler_;
        // server的连接管理器
        IConnManagerPtr conn_mng_;
        int epoll_fd_;
        int listen_fd_;
        static const int ListenID = 0;
        static const int ConnStartID = 1000;
    };
}


#endif