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

// TODO 以后优化给用户定义
int srv_callback(int conn_fd, char *buf, int size) {
    // 回显给客户端
    printf("[conn handle] callback to client.\n");
    if (write(conn_fd, buf, size) == E_FAILED) {
        printf("[conn handle] write error:%s\n", strerror(errno));
        return E_FAILED;
    }
    return E_OK;
}

int Server::Start() {
    printf("[Start] Server listener at ip:%s, port:%d, is starting\n", ip, port);
    int srv_fd = socket(ip_version, SOCK_STREAM, 0);
    if (srv_fd == -1) {
        printf("Server create socket error: %s (code:%d)\n", strerror(errno), errno);
        exit(0);
    }
    struct sockaddr_in srv_addr;
    srv_addr.sin_family = ip_version;
    srv_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    srv_addr.sin_port = htons(port);
    if (bind(srv_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) == -1) {
        printf("bind socket error: %s(code:%d)\n", strerror(errno), errno);
        exit(1);
    }


    if (listen(srv_fd, 20) == -1) {
        printf("listen socket error: %s(code:%d)\n", strerror(errno), errno);
        exit(1);
    }
    printf("Start tink Server %s listening\n", name);
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
        Connection *conn = new Connection();
        conn->Init(cli_fd, cid, this->router);

        int pid = fork();
        if (pid == 0) {
            conn->Start();
//            close(srv_fd);
        }
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

int Server::AddRouter(IRouter *router) {
    this->router = router;
    printf("add router success\n");
    return 0;
}

int Server::Init(char *name, int ip_version, char *ip, int port) {
    strcpy(this->name, name);
    strcpy(this->ip, ip);
    this->ip_version = ip_version;
    this->port = port;
//    this->router = router;
    return 0;
}


