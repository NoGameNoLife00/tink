//
// Created by admin on 2020/5/20.
//

#include <cstdio>
#include <unistd.h>
#include <cstring>
#include "connection.h"
#include "server.h"


int connection::init(int conn_fd, int id, conn_handle_func callback) {
    this->conn_fd = conn_fd;
    this->conn_id = id;
    this->handle_api = callback;
    this->is_close = false;
    return 0;
}

int connection::send(char *buf, int len) {

}

int connection::stop() {
    printf("conn stop, conn_id:%d", conn_id);
    if (is_close) {
        return 0;
    }

    is_close = true;
    // 回收资源
    close(conn_fd);
}

int connection::get_tcp_conn() {
    return this->conn_fd;
}

int connection::get_conn_id() {
    return  this->conn_id;
}

sockaddr connection::get_remote_addr() {
    return this->remote_addr;
}

int connection::start() {
    printf("conn start; conn_id:%d\n", conn_id);
    // 启动当前链接读取数据的业务
    int pid = fork();
    if (pid == 0) {
        start_reader();
    }
    return 0;
}

int connection::start_reader() {
    printf("reader process is running");
    char buf[MAX_MSG_LEN];
    int recv_size = 0;
    while (true) {
        // 读取客户端的数据到buf中
        memset(buf, 0, sizeof(buf));
        if ((recv_size = read(conn_fd, buf, sizeof(buf))) == -1) {
            printf("read error:%s\n", strerror(errno));
            continue;
        }
        int ret = handle_api(conn_fd, buf, recv_size);
        if (ret != E_OK) {
            printf("handle is error, conn_id:%d error_code:%d \n", conn_id, ret);
            break;
        }
    }
    return 0;
}
