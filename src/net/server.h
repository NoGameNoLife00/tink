#ifndef SERVER_H
#define SERVER_H

#include <iserver.h>
#include <string>
#include <memory>
#include <imessage_handler.h>
#include <sys/epoll.h>
#include <unordered_map>

namespace tink {

    class Server : public IServer {
    public:
        int Init(StringPtr &name, int ip_version,
                 StringPtr &ip, int port,
                 IMessageHandlerPtr &&msg_handler);
        int Start();
        int Run();
        int Stop();
        int AddRouter(uint32_t msg_id, std::shared_ptr<IRouter> &router);

    private:
        void handle_accept(int epoll_fd, int listen_fd);
        void handle_events(int epoll_fd, struct epoll_event *events, int event_num, int listen_fd, char *buf);
        void do_read(int epoll_fd, int fd);
        StringPtr name_;
        StringPtr ip_;
        int ip_version_;
        int port_;
        IMessageHandlerPtr msg_handler_;
        typedef std::unordered_map<int, IConnectionPtr> ConnectionMap;
        ConnectionMap conn_map_;
    };
}


#endif