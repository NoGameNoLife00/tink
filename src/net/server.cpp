#include <server.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <cstring>
#include <unistd.h>
#include <connection.h>
#include <global_mng.h>
#include <scope_guard.h>
#include <type.h>
#include <conn_manager.h>
#include <message.h>
#include <datapack.h>
#include <request.h>


#define MAX_BUF_SIZE 2048
namespace tink {
    int Server::Init(string &name, int ip_version,
                     string &ip, int port,
                     IMessageHandlerPtr &&msg_handler) {
        name_ = name;
        listen_addr_ = std::make_shared<SockAddress>(ip, port, ip_version == AF_INET6);
        msg_handler_ = msg_handler;
        conn_mng_ = std::make_shared<ConnManager>();
        return 0;
    }


    int Server::Start() {
        auto globalObj = GlobalInstance;
        logger->info("[tink] Server Name:%v, listener at IP:%v, Port:%v, is starting.\n",
                name_, listen_addr_->ToIp(), listen_addr_->ToPort());
        logger->info("[tink] Version: %v, MaxConn:%v, MaxPacketSize:%v\n", globalObj->GetVersion().c_str(),
               globalObj->GetMaxConn(), globalObj->GetMaxPackageSize());

        // 开启worker工作池
        msg_handler_->StartWorkerPool();

//        listen_fd_ = socket(ip_version_, SOCK_STREAM, 0);

//        SockAddress listen_addr(port_, false,ip_version_ == AF_INET6);
        listen_socket_ = std::make_unique<Socket>(SocketApi::Create(listen_addr_->Family()));
        listen_socket_->BindAddress(*listen_addr_);
        listen_socket_->Listen();
//        ON_SCOPE_EXIT([&]{
//            close(listen_fd_);
//        });

//        struct sockaddr_in srv_addr;
//        srv_addr.sin_family = ip_version_;
//        inet_pton(ip_version_, "0.0.0.0", &srv_addr.sin_addr);
//        srv_addr.sin_port = htons(port_);
//        if (bind(listen_fd_, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) == -1) {
//            logger->info("bind socket error: %v(code:%v)\n", strerror(errno), errno);
//            exit(1);
//        }

//        if (listen(listen_fd_, 20) == -1) {
//            logger->info("listen socket error: %v(code:%v)\n", strerror(errno), errno);
//            exit(1);
//        }
        logger->info("Start tink Server %v listening\n", name_.c_str());
        epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
        OperateEvent(listen_socket_->GetSockFd(), ListenID, EPOLL_CTL_ADD, EPOLLIN);
        int fd_num;
        for ( ; ; ) {
            fd_num = epoll_wait(epoll_fd_, &*events_.begin(), events_.size(), -1);
            HandleEvents_(fd_num);
        }
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

    void Server::HandleEvents_(int event_num) {
        int id;
        for (int i = 0; i < event_num; i++) {
            id = events_[i].data.u32;
            uint32_t e = events_[i].events;
            if ((id == ListenID) && (e & EPOLLIN)) {
                HandleAccept_();
            } else if( e & EPOLLERR || e & EPOLLHUP || e & EPOLLRDHUP || (!e & EPOLLIN)) {
                DoError_(id);
            } else if (e & EPOLLIN) {
                DoRead_(id);
            } else if (e & EPOLLOUT) {
                DoWrite_(id);
            }
        }
    }

    void Server::HandleAccept_()
    {
        static int cid = ConnStartID;
        int cli_fd;
//        RemoteAddrPtr cli_addr = std::make_shared<sockaddr>();
        SockAddressPtr cli_addr = std::make_shared<SockAddress>();
//        socklen_t  cli_addr_len;
//        memset(cli_addr.get(), 0, sizeof(sockaddr));
//        cli_fd = accept(listen_fd, cli_addr.get(), &cli_addr_len);
        cli_fd = listen_socket_->Accept(*cli_addr);
        if (cli_fd == -1) {
            logger->warn("accept socket error: %v(code:%v)\n", strerror(errno), errno);
        } else {
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
            OperateEvent(cli_fd, cid, EPOLL_CTL_ADD, EPOLLIN);
        }
    }

    void Server::DoError_(int id) {
        auto conn = conn_mng_->Get(id);
        if (!conn) {
            logger->warn("close not find conn, id=%v", id);
            return;
        }
        logger->warn("conn error, id=%v", id);
        OperateEvent(conn->GetTcpConn(), id, EPOLL_CTL_DEL, EPOLLIN);
        conn->Stop();
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

        auto on_error = [&fd, &id, &conn, this]  {
            OperateEvent(fd, id, EPOLL_CTL_DEL, EPOLLIN);
            conn->Stop();
        };
        // 读取客户端发送的包头
        int ret = read(fd, head_data.get(), head_len);
        if (ret == -1) {
            logger->error("[reader] msg head error:%v\n", strerror(errno));
            on_error();
            return;
        } else if (ret == 0) {
            logger->warn("[reader] client close");
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
        std::lock_guard<Mutex> guard(conn->GetMutex());
        auto& buffer = conn->GetBuffer();
        ret = write(fd, buffer->Data(), buffer->Length());
        buffer->Reset();
        if (ret == -1)
        {
            logger->error("[writer] error:%v\n", strerror(errno));
            close(fd);
            OperateEvent(fd, id, EPOLL_CTL_DEL, EPOLLOUT);
        }
        else
            OperateEvent(fd, id, EPOLL_CTL_MOD, EPOLLIN);
    }

    void Server::OperateEvent(uint32_t fd, uint32_t id, int op, int state) {
        struct epoll_event ev {0};
        ev.events = state;
        ev.data.u32 = id;
        epoll_ctl(epoll_fd_, op, fd, &ev);
    }


}
