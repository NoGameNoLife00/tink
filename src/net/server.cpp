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

namespace tink {

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
        srv_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
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

        while (true) {
            RemoteAddrPtr cli_addr(new sockaddr);
            socklen_t cli_add_size = sizeof(sockaddr);
            int cli_fd = accept(srv_fd, cli_addr.get(), &cli_add_size);
            if (cli_fd == -1) {
                logger->info("accept socket error: %v(code:%v)\n", strerror(errno), errno);
                continue;
            }
            // 判断最大连接数
            if (conn_mng_->Size() >= GlobalInstance->GetMaxConn()) {
                // TODO 发送连接失败消息
                logger->warn("too many connections max_conn=%v", GlobalInstance->GetMaxConn());
                close(cli_fd);
                continue;
            }

            cid++;
            std::shared_ptr<Connection> conn(new Connection);
            conn->Init(std::dynamic_pointer_cast<IServer>(shared_from_this()), cli_fd, cid, this->msg_handler_, cli_addr);
            conn->Start();
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

}
