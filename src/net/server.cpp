#include <server.h>
#include <errno.h>
#include <cstring>
#include <unistd.h>
#include <connection.h>
#include <config_mng.h>
#include <scope_guard.h>
#include <common.h>
#include <conn_manager.h>
#include <message.h>
#include <datapack.h>
#include <request.h>
#include <context.h>
#include <context_manage.h>
#include <signal.h>
#include <daemon.h>
#include <module_manage.h>
#include "harbor.h"
#include "timer.h"
#include "monitor.h"


#define MAX_BUF_SIZE 2048
namespace tink {
    static volatile int SIG = 0;

    static void HandleHup(int signal) {
        if (signal == SIGHUP) {
            SIG = 1;
        }
    }

    int Sigign() {
        struct sigaction sa;
        sa.sa_handler = SIG_IGN;
        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGPIPE, &sa, 0);
        return 0;
    }


    int Server::Init(string &name, int ip_version,
                     string &ip, int port,
                     MessageHandlerPtr &&msg_handler) {
        name_ = name;
        listen_addr_ = std::make_shared<SockAddress>(ip, port, ip_version == AF_INET6);
        msg_handler_ = msg_handler;
        conn_mng_ = std::make_shared<ConnManager>();
        CurrentHandle::InitThread(THREAD_MAIN);
//        Sigign();

        // register SIGHUP for log file reopen
//        struct sigaction sa;
//        sa.sa_handler = &HandleHup;
//        sa.sa_flags = SA_RESTART;
//        sigfillset(&sa.sa_mask);
//        sigaction(SIGHUP, &sa, NULL);
        if (!ConfigMngInstance.GetDaemon().empty()) {
            if (Daemon::Init(ConfigMngInstance.GetDaemon())) {
                exit(1);
            }
        }
        Harbor::Init(ConfigMngInstance.GetHarbor());
        ContextMngInstance.Init(ConfigMngInstance.GetHarbor());
        ModuleMngInstance.Init(ConfigMngInstance.GetModulePath());
        TimerInstance.Init();

        return 0;
    }


    static void wakeup(MonitorPtr m, int busy) {
        if (m->sleep >= m->count - busy) {
            // signal sleep worker, "spurious wakeup" is harmless
            m->cond.notify_one();
        }
    }


#define CHECK_ABORT if (Context::Total()==0) break;
    static void ThreadMonitor(MonitorPtr m) {
        int i;
        int n = m->count;
        CurrentHandle::InitThread(THREAD_MONITOR);
        for (;;) {
            CHECK_ABORT
            for (i=0;i<n;i++) {
                m->m[i]->Check();
            }
            for (i=0;i<5;i++) {
                CHECK_ABORT
                sleep(1);
            }
        }
    }


    static void SignalHup() {
        // make log file reopen

        MsgPtr smsg = std::make_shared<Message>();
        smsg->source = 0;
        smsg->session = 0;
        smsg->data = NULL;
        smsg->size = static_cast<size_t>(PTYPE_SYSTEM) << MESSAGE_TYPE_SHIFT;
        uint32_t logger = ContextMngInstance.FindName("logger");
        if (logger) {
            ContextMngInstance.PushMessage(logger, smsg);
        }
    }

    static void ThreadTimer(MonitorPtr m) {
        CurrentHandle::InitThread(THREAD_TIMER);
        for (;;) {
            TimerInstance.UpdateTime();
            // socket_updatetime();
            CHECK_ABORT
            wakeup(m,m->count-1);
            usleep(2500);
            if (SIG) {
                SignalHup();
                SIG = 0;
            }
        }
        // wakeup socket thread
        // socket_exit();
        // wakeup all worker thread
        m->mutex.lock();
        m->quit = 1;
        m->cond.notify_all();
        m->mutex.unlock();
        return;
    }


    static void ThreadSocket(MonitorPtr m) {

    }

    int Server::Start() {
        auto& config = ConfigMngInstance;
        spdlog::info("[tink] Server Name:{}, listener at IP:{}, Port:{}, is starting.",
                name_, listen_addr_->ToIp(), listen_addr_->ToPort());
        spdlog::info("[tink] Version: {}, MaxConn:{}, MaxPacketSize:{}", config.GetVersion().c_str(),
                     config.GetMaxConn(), config.GetMaxPackageSize());

        int thread = config.GetWorkerPoolSize();
        MonitorPtr m = std::shared_ptr<Monitor>();
        m->count = thread;
        m->sleep = 0;
        for (int i = 0; i < thread; i++) {
            m->m.emplace_back(std::make_shared<MonitorNode>());
        }

        std::unique_ptr<Thread> t_monitor = std::make_unique<Thread>(std::bind(ThreadMonitor, m), "monitor");
        std::unique_ptr<Thread> t_timer = std::make_unique<Thread>(std::bind(ThreadTimer, m), "timer");
        std::unique_ptr<Thread> t_socket = std::make_unique<Thread>(std::bind(ThreadSocket, m), "timer");




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
            if (conn_mng_->Size() >= ConfigMngInstance.GetMaxConn()) {
                // TODO 发送连接失败消息
                spdlog::warn("too many connections max_conn={}", ConfigMngInstance.GetMaxConn());
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
        UBytePtr head_data = std::make_unique<byte[]>(head_len);
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
            UBytePtr buf = std::make_unique<byte[]>(msg->GetDataLen());
            if ((read(fd, buf.get(), msg->GetDataLen()) == -1)) {
                spdlog::warn("[reader] msg data error:{}", strerror(errno));
                on_error();
                return;
            }
            msg->SetData(buf);
        }

        RequestPtr req_ptr = std::make_shared<Request>(conn,msg);
        if (ConfigMngInstance.GetWorkerPoolSize() > 0) {
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
