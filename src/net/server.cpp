#include <server.h>
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
                     MessageHandlerPtr &&msg_handler) {
        name_ = name;
        listen_addr_ = std::make_shared<SockAddress>(ip, port, ip_version == AF_INET6);
        msg_handler_ = msg_handler;
        conn_mng_ = std::make_shared<ConnManager>();
        return 0;
    }


    int Server::Start() {
        auto& globalObj = GlobalInstance;
        spdlog::info("[tink] Server Name:{}, listener at IP:{}, Port:{}, is starting.",
                name_, listen_addr_->ToIp(), listen_addr_->ToPort());
        spdlog::info("[tink] Version: {}, MaxConn:{}, MaxPacketSize:{}", globalObj.GetVersion().c_str(),
               globalObj.GetMaxConn(), globalObj.GetMaxPackageSize());

        // 开启worker工作池
        msg_handler_->StartWorkerPool();
        listen_socket_ = std::make_unique<Socket>(SocketApi::Create(listen_addr_->Family()));
        listen_socket_->BindAddress(*listen_addr_);
        listen_socket_->Listen();
        spdlog::info("Start tink Server {} listening", name_.c_str());
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
        spdlog::info("[Stop] tink server name %s", name_);
        conn_mng_->ClearConn();
        return 0;
    }

    int Server::AddRouter(uint32_t msg_id, std::shared_ptr<BaseRouter> &router) {
        msg_handler_->AddRouter(msg_id, router);
        return 0;
    }


    void Server::CallOnConnStop(ConnectionPtr &&conn) {
        if (on_conn_stop_) {
            spdlog::info("[call] on_conn_stop ..");
            on_conn_stop_(conn);
        }

    }

    void Server::CallOnConnStart(ConnectionPtr &&conn) {
        if (on_conn_start_) {
            spdlog::info("[call] on_conn_start ..");
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
        SockAddressPtr cli_addr = std::make_shared<SockAddress>();
        cli_fd = listen_socket_->Accept(*cli_addr);
        if (cli_fd == -1) {
            spdlog::warn("accept socket error: {}(code:{})", strerror(errno), errno);
        } else {
            // 判断最大连接数
            if (conn_mng_->Size() >= GlobalInstance.GetMaxConn()) {
                // TODO 发送连接失败消息
                spdlog::warn("too many connections max_conn={}", GlobalInstance.GetMaxConn());
                close(cli_fd);
                return;
            }
            cid++;
            ConnectionPtr conn(new Connection);
			conn->Init(std::dynamic_pointer_cast<Server>(shared_from_this()), cli_fd, cid, this->msg_handler_, cli_addr);
            conn->Start();
            //添加一个客户描述符和事件
            OperateEvent(cli_fd, cid, EPOLL_CTL_ADD, EPOLLIN);
        }
    }

    void Server::DoError_(int id) {
        auto conn = conn_mng_->Get(id);
        if (!conn) {
            spdlog::warn("close not find conn, id={}", id);
            return;
        }
        spdlog::warn("conn error, id={}", id);
        OperateEvent(conn->GetTcpConn(), id, EPOLL_CTL_DEL, EPOLLIN);
        conn->Stop();
    }

    void Server::DoRead_(int id)
    {
        int head_len = DataPack::GetHeadLen();
        BytePtr head_data = std::make_unique<byte[]>(head_len);
        memset(head_data.get(), 0, head_len);
        // 读取客户端的数据到buf中
        MessagePtr msg = std::make_unique<NetMessage>();


        auto conn = conn_mng_->Get(id);
        if (!conn) {
            spdlog::error("[reader] not find connect:{}", id);
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
            spdlog::error("[reader] msg head error:{}", strerror(errno));
            on_error();
            return;
        } else if (ret == 0) {
            spdlog::warn("[reader] client close");
            on_error();
            return;
        }
        ret = DataPack::Unpack(head_data, *msg.get());
        if (ret != E_OK) {
            spdlog::warn("[reader] unpack error: {}", ret);
            on_error();
            return;
        }
        // 根据dataLen，再读取Data,放入msg中
        if (msg->GetDataLen() > 0) {
            spdlog::debug("msg data len:{}", msg->GetDataLen());
            BytePtr buf = std::make_unique<byte[]>(msg->GetDataLen());
            if ((read(fd, buf.get(), msg->GetDataLen()) == -1)) {
                spdlog::warn("[reader] msg data error:{}", strerror(errno));
                on_error();
                return;
            }
            msg->SetData(buf);
        }

        RequestPtr req_ptr = std::make_shared<Request>(conn,msg);
        if (GlobalInstance.GetWorkerPoolSize() > 0) {
            conn->GetMsgHandler()->SendMsgToTaskQueue(req_ptr);
        }
    }

    void Server::DoWrite_(int id)
    {
        int ret;
        auto conn = conn_mng_->Get(id);
        if (!conn) {
            spdlog::warn("[write] not find conn, id={}", id);
            return;
        }
        int fd = conn->GetTcpConn();
        std::lock_guard<Mutex> guard(conn->GetMutex());
        auto& buffer = conn->GetBuffer();
        ret = write(fd, buffer->Data(), buffer->Length());
        buffer->Reset();
        if (ret == -1)
        {
            spdlog::error("[writer] error:{}", strerror(errno));
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
