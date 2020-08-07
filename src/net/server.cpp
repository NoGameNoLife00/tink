#include <server.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <cstring>
#include <unistd.h>
#include <connection.h>
#include <global_mng.h>
#include <scope_guard.h>
#include <type.h>
#include <message.h>
#include "datapack.h"
#include "request.h"

#define EPOLL_EVENTS_NUM 100
#define MAX_BUF_SIZE 2048
namespace tink {

    static void add_event(int epoll_fd, int fd, int state)
    {
        struct epoll_event ev;
        ev.events = state;
        ev.data.fd = fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    }

    static void delete_event(int epoll_fd, int fd, int state)
    {
        struct epoll_event ev;
        ev.events = state;
        ev.data.fd = fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &ev);
    }

    static void modify_event(int epoll_fd, int fd, int state)
    {
        struct epoll_event ev;
        ev.events = state;
        ev.data.fd = fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
    }



    static void do_write(int epollfd,int fd,char *buf)
    {
        int nwrite;
        nwrite = write(fd,buf,strlen(buf));
        if (nwrite == -1)
        {
            perror("write error:");
            close(fd);
            delete_event(epollfd,fd,EPOLLOUT);
        }
        else
            modify_event(epollfd,fd,EPOLLIN);
        memset(buf,0,MAX_BUF_SIZE);
    }


    int Server::Start() {
        auto globalObj = GlobalInstance;
        logger->info("[tink] Server Name:%v, listener at IP:%v, Port:%v, is starting.\n",
                name_->c_str(), ip_->c_str(), port_);
        logger->info("[tink] Version: %v, MaxConn:%v, MaxPacketSize:%v\n", globalObj->GetVersion()->c_str(),
               globalObj->GetMaxConn(), globalObj->GetMaxPackageSize());

        // 开启worker工作池
        msg_handler_->StartWorkerPool();

        int srv_fd = socket(ip_version_, SOCK_STREAM, 0);
        if (srv_fd == -1) {
            logger->info("Server create socket error: %v (code:%v)\n", strerror(errno), errno);
            exit(0);
        }
        ON_SCOPE_EXIT([&]{
            close(srv_fd);
        });

        struct sockaddr_in srv_addr;
        srv_addr.sin_family = ip_version_;
        inet_pton(ip_version_, "0.0.0.0", &srv_addr.sin_addr);
//        srv_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
        srv_addr.sin_port = htons(port_);
        if (bind(srv_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) == -1) {
            logger->info("bind socket error: %v(code:%v)\n", strerror(errno), errno);
            exit(1);
        }

        if (listen(srv_fd, 20) == -1) {
            logger->info("listen socket error: %v(code:%v)\n", strerror(errno), errno);
            exit(1);
        }
        logger->info("Start tink Server %v listening\n", name_.get()->c_str());

        u_int cid = 0;

        int epoll_fd;
        epoll_fd = epoll_create1(EPOLL_CLOEXEC);
        struct epoll_event events[EPOLL_EVENTS_NUM];
        byte *buf = new byte [MAX_BUF_SIZE];
        add_event(epoll_fd, srv_fd, EPOLLIN);
        int fd_num;
        for ( ; ; ) {
            fd_num = epoll_wait(epoll_fd, events, EPOLL_EVENTS_NUM, -1);
            handle_events(epoll_fd, events, fd_num, srv_fd, buf);
        }

        for(;;) {
            RemoteAddrPtr cli_addr(new sockaddr);
             socklen_t cli_add_size = sizeof(sockaddr);
            int cli_fd = accept(srv_fd, cli_addr.get(), &cli_add_size);
            if (cli_fd == -1) {
                logger->info("accept socket error: %v(code:%v)\n", strerror(errno), errno);
                continue;
            }
            cid++;
            std::shared_ptr<Connection> conn(new Connection);
            conn->Init(cli_fd, cid, this->msg_handler_, cli_addr);
            conn->Start();
        }
        return 0;
    }

    int Server::Run() {
        Start();
        return 0;
    }

    int Server::Stop() {
        return 0;
    }

    int Server::AddRouter(uint32_t msg_id, std::shared_ptr<IRouter> &router) {
        msg_handler_->AddRouter(msg_id, router);
        return 0;
    }

    int Server::Init(StringPtr &name, int ip_version,
                     StringPtr &ip, int port,
                     IMessageHandlerPtr &&msg_handler) {
        this->name_ = name;
        this->ip_ = ip;
        this->ip_version_ = ip_version;
        this->port_ = port;
        this->msg_handler_ = msg_handler;
        return 0;
    }

    void Server::handle_events(int epoll_fd, struct epoll_event *events, int event_num, int listen_fd, char *buf) {
        int fd;
        for (int i = 0; i < event_num; i++) {
            fd = events[i].data.fd;
            if ((fd == listen_fd) && (events[i].events & EPOLLIN)) {
                handle_accept(epoll_fd,listen_fd);
            } else if (events[i].events & EPOLLIN) {
                do_read(epoll_fd, fd);
            } else if (events[i].events & EPOLLOUT) {
                do_write(epoll_fd, fd,buf);
            }
        }
    }

    void Server::handle_accept(int epoll_fd, int listen_fd)
    {
        static int cid = 0;
        int cli_fd;
        RemoteAddrPtr cli_addr(new sockaddr);
        socklen_t  cli_addr_len;
        cli_fd = accept(listen_fd, (struct sockaddr*)&cli_addr, &cli_addr_len);
        if (cli_fd == -1)
            logger->warn("accept socket error: %v(code:%v)\n", strerror(errno), errno);
        else
        {
            cid++;
            ConnectionPtr conn(new Connection);
            conn->Init(cli_fd, cid, msg_handler_, cli_addr);
            //添加一个客户描述符和事件
            add_event(epoll_fd, cli_fd, EPOLLIN);
            conn_map_.insert(std::pair<int, ConnectionPtr>(cli_fd, conn));
        }
    }

    void Server::do_read(int epoll_fd, int fd)
    {
        int head_len = DataPack::GetHeadLen();
        byte head_data[head_len];
        // 读取客户端的数据到buf中
        IMessagePtr msg(new Message);
        memset(head_data, 0, head_len);

        auto on_error = [&fd, &epoll_fd]  {
            close(fd);
            delete_event(epoll_fd, fd, EPOLLIN);
        };

        auto it = conn_map_.find(fd);
        if (it == conn_map_.end()) {
            logger->error("[reader] not find connect:%v");
            on_error();
            return;
        }
        IConnectionPtr conn = it->second;
        // 读取客户端发送的包头
        int ret = read(fd, head_data, head_len);
        if (ret == -1) {
            logger->error("[reader] msg head error:%v\n", strerror(errno));
            on_error();
            return;
        } else if (ret == 0) {
            logger->error("[reader] client close");
            on_error();
            return;
        }
        ret = DataPack::Unpack(head_data, *msg.get());
        if (ret != E_OK) {
            logger->warn("[reader] unpack error: %v\n", ret);
            on_error();
            return;
        }
        // 根据dataLen，再读取Data,放入msg中
        if (msg->GetDataLen() > 0) {
            BytePtr buf(new byte[msg->GetDataLen()] {0});
            if ((read(fd, buf.get(), msg->GetDataLen()) == -1)) {
                logger->warn("[reader] msg data error:%v\n", strerror(errno));
                on_error();
                return;
            }
            msg->SetData(buf);
        }
        modify_event(epoll_fd, fd, EPOLLOUT);

        IRequestPtr req_ptr = std::make_shared<Request>(conn, msg);
        if (GlobalInstance->GetWorkerPoolSize() > 0) {
            conn->GetMsgHandler()->SendMsgToTaskQueue(req_ptr);
        } else {
            //msg_handler_->DoMsgHandle(req);
        }

    }
}
