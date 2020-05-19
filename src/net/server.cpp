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



int server::start() {
    printf("[start] server listener at ip:%s, port:%d, is starting\n", ip, port);
    int srv_fd = socket(ip_version, SOCK_STREAM, 0);
    if (srv_fd == -1) {
        printf("server create socket error: %s (code:%d)", strerror(errno), errno);
        exit(0);
    }
    struct sockaddr_in srv_addr;
    srv_addr.sin_family = ip_version;
    srv_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    srv_addr.sin_port = htons(port);
    if (bind(srv_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) == -1) {
        printf("bind socket error: %s(code:%d)", strerror(errno), errno);
    }


    if (listen(srv_fd, 20) == -1) {
        printf("listen socket error: %s(code:%d)", strerror(errno), errno);
    }
    printf("start tink server %s listening\n", name);
    struct sockaddr_in cli_addr;
    socklen_t cli_add_size = sizeof(cli_addr);
    while (true) {
        int cli_fd = accept(srv_fd, (struct sockaddr*)&cli_addr, &cli_add_size);
        if (cli_fd == -1) {
            printf("accept socket error: %s(code:%d)", strerror(errno), errno);
            continue;
        }
        int pid = fork();
        if (pid == 0) {
            char recv_buf[MAX_MSG_LEN] = {};
            while (true) {
                int ret = read(cli_fd, recv_buf, sizeof(recv_buf));
                if (ret == 0) {
                    printf("client socket closed \n");
                    break;
                } else if (ret < 0) {
                    printf("read client error: %s(code:%d)", strerror(errno), errno);
                    break;
                }
                printf("recv client message: %s \n", recv_buf);
                write(cli_fd, recv_buf, ret);
                memset(recv_buf, 0, sizeof(recv_buf));
            }
            close(cli_fd);
            close(srv_fd);
            exit(0);
        } else{
            close(cli_fd);
        }
    }
    close(srv_fd);
    return 0;
}

int server::run() {
    start();
    return 0;
}

int server::stop() {
    return 0;
}

int server::init(char *name, int ip_version, char *ip, int port) {
    strcpy(this->name, name);
    strcpy(this->ip, ip);
    this->ip_version = ip_version;
    this->port = port;
    return 0;
}


server *new_server() {
    server *s = new server();
    s->init("tink", AF_INET, "0.0.0.0", 8823);
    return s;
}
