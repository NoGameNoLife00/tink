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
#include "conn_manager.h"
#include <message.h>
#include <datapack.h>
#include <request.h>

#define EPOLL_EVENTS_NUM 100
#define MAX_BUF_SIZE 2048
namespace tink {
    int Server::Start() {
        auto globalObj = GlobalInstance;
        logger->info("[tink] Server Name:%v, listener at IP:%v, Port:%v, is starting.\n",
                name_->c_str(), ip_->c_str(), port_);
        logger->info("[tink] Version: %v, MaxConn:%v, MaxPacketSize:%v\n", globalObj->GetVersion()->c_str(),
               globalObj->GetMaxConn(), globalObj->GetMaxPackageSize());

        // 开启worker工作池
        msg_handler_->StartWorkerPool();

        listen_fd_ = socket(ip_version_, SOCK_STREAM, 0);
        if (listen_fd_ == -1) {
            logger->info("Server create socket error: %v (code:%v)\n", strerror(errno), errno);
            exit(0);
        }
        ON_SCOPE_EXIT([&]{
            close(listen_fd_);
        });

        struct sockaddr_in srv_addr;
        srv_addr.sin_family = ip_version_;
        inet_pton(ip_version_, "0.0.0.0", &srv_addr.sin_addr);
//        srv_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
        srv_addr.sin_port = htons(port_);
        if (bind(listen_fd_, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) == -1) {
            logger->info("bind socket error: %v(code:%v)\n", strerror(errno), errno);
            exit(1);
        }

        if (listen(listen_fd_, 20) == -1) {
            logger->info("listen socket error: %v(code:%v)\n", strerror(errno), errno);
            exit(1);
        }
        logger->info("Start tink Server %v listening\n", name_.get()->c_str());




        epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
        struct epoll_event events[EPOLL_EVENTS_NUM];
        OperateEvent(listen_fd_, ListenID, EPOLL_CTL_ADD, EPOLLIN);
        int fd_num;
        for ( ; ; ) {
            fd_num = epoll_wait(epoll_fd_, events, EPOLL_EVENTS_NUM, -1);
            HandleEvents_(events, fd_num);
        }

//        for(;;) {
//            RemoteAddrPtr cli_addr(new sockaddr);
//             socklen_t cli_add_size = sizeof(sockaddr);
//            int cli_fd = accept(listen_fd_, cli_addr.get(), &cli_add_size);
//            if (cli_fd == -1) {
//                logger->info("accept socket error: %v(code:%v)\n", strerror(errno), errno);
//                continue;
//            }
//            cid++;
//            std::shared_ptr<Connection> conn(new Connection);
//            conn->Init(cli_fd, cid, this->msg_handler_, cli_addr);
//            conn->Start();
//        }
        return 0;
    }

    int Server::Run() {
        Start();
        return 0;
    }

    int Server::Stop() {
        logger->info("[Stop] tink server name %s", name_);
        conn_mng_->ClearConn();
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
        this->conn_mng_ = std::make_shared<ConnManager>();

        return 0;
    }

    void Server::CallOnConnStop(IConnectionPtr &&conn) {
        if (on_conn_stop_) {
            logger->info("[call] on_conn_stop ..");
            on_conn_stop_(conn);
        }

    }

    void Server::CallOnConnStart(IConnectionPtr &&conn) {
        if (on_conn_start_) {
            logger->info("[call] on_conn_start ..");
            on_conn_start_(conn);
        }
    }

    void Server::SetOnConnStop(ConnHookFunc &&func) {
        on_conn_stop_ = std::forward<ConnHookFunc>(func);
    }

    void Server::SetOnConnStart(ConnHookFunc &&func) {
        on_conn_start_ = std::forward<ConnHookFunc>(func);
    }

    void Server::HandleEvents_(struct epoll_event *events, int event_num) {
        int id;
        for (int i = 0; i < event_num; i++) {
            id = events[i].data.u32;
            if ((id == ListenID) && (events[i].events & EPOLLIN)) {
                HandleAccept_(listen_fd_);
            } else if (events[i].events & EPOLLIN) {
                DoRead_(id);
            } else if (events[i].events & EPOLLOUT) {
                DoWrite_(id);
            }
        }
    }

    void Server::HandleAccept_(int listen_fd)
    {
        static int cid = ConnStartID;
        int cli_fd;
        RemoteAddrPtr cli_addr(new sockaddr);
        socklen_t  cli_addr_len;
        cli_fd = accept(listen_fd, (struct sockaddr*)&cli_addr, &cli_addr_len);
        if (cli_fd == -1)
            logger->warn("accept socket error: %v(code:%v)\n", strerror(errno), errno);
        else
        {
            // 判断最大连接数
            if (conn_mng_->Size() >= GlobalInstance->GetMaxConn()) {
                // TODO 发送连接失败消息
                logger->warn("too many connections max_conn=%v", GlobalInstance->GetMaxConn());
                close(cli_fd);
                return;
            }
            cid++;
            ConnectionPtr conn(new Connection);
			conn->Init(std::dynamic_pointer_cast<IServer>(shared_from_this()), cli_fd, cid, this->msg_handler_, cli_addr);
            conn->Start();
            //添加一个客户描述符和事件
            OperateEvent(cli_fd, 0, EPOLL_CTL_ADD, EPOLLIN);
        }
    }

    void Server::DoRead_(int id)
    {
        int head_len = DataPack::GetHeadLen();
        BytePtr head_data = std::make_unique<byte[]>(head_len);
        memset(head_data.get(), 0, head_len);
        // 读取客户端的数据到buf中
        IMessagePtr msg = std::make_unique<Message>();


        auto conn = conn_mng_->Get(id);
        if (!conn) {
            logger->error("[reader] not find connect:%v", id);
            return;
        }
        int fd = conn->GetTcpConn();

        auto on_error = [&fd, &id, this]  {
            close(fd);
            OperateEvent(fd, id, EPOLL_CTL_DEL, EPOLLIN);
        };
        // 读取客户端发送的包头
        int ret = read(fd, head_data.get(), head_len);
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
            logger->debug("msg data len:%v", msg->GetDataLen());
            BytePtr buf = std::make_unique<byte[]>(msg->GetDataLen());
            if ((read(fd, buf.get(), msg->GetDataLen()) == -1)) {
                logger->warn("[reader] msg data error:%v\n", strerror(errno));
                on_error();
                return;
            }
            msg->SetData(buf);
        }
//        modify_event(epoll_fd_, fd, EPOLLOUT);

        IRequestPtr req_ptr = std::make_shared<Request>(conn,msg);
        if (GlobalInstance->GetWorkerPoolSize() > 0) {
            conn->GetMsgHandler()->SendMsgToTaskQueue(req_ptr);
        } else {
            //msg_handler_->DoMsgHandle(req);
        }

    }

    void Server::DoWrite_(int id)
    {
        int ret;
        auto conn = conn_mng_->Get(id);
        if (!conn) {
            logger->warn("[write] not find conn, id=%v", id);
            return;
        }
        int fd = conn->GetTcpConn();
        std::lock_guard<std::mutex> guard(conn->GetMutex());
        ret = write(fd, conn->GetBuffer().get(), conn->GetBuffOffset());
        conn->SetBuffOffset(0);
        memset(conn->GetBuffer().get(), 0, conn->GetBufferLen());
        if (ret == -1)
        {
            logger->error("[writer] error:%v\n", strerror(errno));
            close(fd);
            OperateEvent(fd, id, EPOLL_CTL_DEL, EPOLLOUT);
        }
        else
            OperateEvent(fd, id, EPOLL_CTL_MOD, EPOLLIN);
        // delete buff???
    }

    void Server::OperateEvent(uint32_t fd, uint32_t id, int op, int state) {
        struct epoll_event ev;
        ev.events = state;
        ev.data.u32 = id;
        epoll_ctl(epoll_fd_, op, fd, &ev);
    }
}
