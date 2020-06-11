//
// Created by admin on 2020/5/14.
//

#include <server.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <cstring>
#include <unistd.h>
#include <connection.h>
#include <global_mng.h>


namespace tink {


    int Server::Start() {
        std::shared_ptr<GlobalMng> globalObj = tink::Singleton<tink::GlobalMng>::GetInstance();
        printf("[tink] Server Name:%s, listener at IP:%s, Port:%d, is starting.\n",
                name_->c_str(), ip_->c_str(), port_);
        printf("[tink] Version: %s, MaxConn:%d, MaxPacketSize:%d\n", globalObj->GetVersion()->c_str(),
               globalObj->GetMaxConn(), globalObj->GetMaxPackageSize());
        int srv_fd = socket(ip_version_, SOCK_STREAM, 0);
        if (srv_fd == -1) {
            printf("Server create socket error: %s (code:%d)\n", strerror(errno), errno);
            exit(0);
        }
        struct sockaddr_in srv_addr;
        srv_addr.sin_family = ip_version_;
        srv_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
        srv_addr.sin_port = htons(port_);
        if (bind(srv_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) == -1) {
            printf("bind socket error: %s(code:%d)\n", strerror(errno), errno);
            exit(1);
        }


        if (listen(srv_fd, 20) == -1) {
            printf("listen socket error: %s(code:%d)\n", strerror(errno), errno);
            exit(1);
        }
        printf("Start tink Server %s listening\n", name_.get()->c_str());
        struct sockaddr_in cli_addr;
        u_int cid = 0;
        socklen_t cli_add_size = sizeof(cli_addr);
        while (true) {
            int cli_fd = accept(srv_fd, (struct sockaddr*)&cli_addr, &cli_add_size);
            if (cli_fd == -1) {
                printf("accept socket error: %s(code:%d)\n", strerror(errno), errno);
                continue;
            }
            cid++;
            std::shared_ptr<Connection> conn(new Connection);
            conn->Init(cli_fd, cid, this->msg_handler_);
            conn->Start();
//        int pid = fork();
//        if (pid == 0) {
//            char recv_buf[MAX_MSG_LEN] = {};
//            while (true) {
//                int ret = read(cli_fd, recv_buf, sizeof(recv_buf));
//                if (ret == 0) {
//                    printf("client socket closed \n");
//                    break;
//                } else if (ret < 0) {
//                    printf("read client error: %s(code:%d)", strerror(errno), errno);
//                    break;
//                }
//                printf("recv client message: %s \n", recv_buf);
//                write(cli_fd, recv_buf, ret);
//                memset(recv_buf, 0, sizeof(recv_buf));
//            }
//            close(cli_fd);
//            close(srv_fd);
//            exit(0);
//        } else{
//            close(cli_fd);
//        }
        }
        close(srv_fd);
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

    int Server::Init(std::shared_ptr<std::string> name, int ip_version,
                     std::shared_ptr<std::string> ip, int port,
                     std::shared_ptr<IMessageHandler> &msg_handler) {
        this->name_ = name;
        this->ip_ = ip;
        this->ip_version_ = ip_version;
        this->port_ = port;
        this->msg_handler_ = msg_handler;
        return 0;
    }



}
