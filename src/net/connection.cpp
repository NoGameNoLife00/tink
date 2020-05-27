//
// Created by admin on 2020/5/20.
//

#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <connection.h>
#include <server.h>
#include "request.h"


int Connection::Init(int conn_fd, int id) {
    this->conn_fd = conn_fd;
    this->conn_id = id;
    this->is_close = false;
    return 0;
}

int Connection::Send(char *buf, int len) {

}

int Connection::Stop() {
    printf("conn Stop, conn_id:%d\n", conn_id);
    if (is_close) {
        return 0;
    }

    is_close = true;
    // 回收资源
    close(conn_fd);
}

int Connection::GetTcpConn() {
    return this->conn_fd;
}

int Connection::GetConnId() {
    return  this->conn_id;
}

sockaddr Connection::GetRemoteAddr() {
    return this->remote_addr;
}

int Connection::Start() {
    printf("conn Start; conn_id:%d\n", conn_id);
    // 启动当前链接读取数据的业务
    int pid = fork();
    if (pid == 0) {
        StartReader();
    }
    return 0;
}

int Connection::StartReader() {
    printf("reader process is running\n");
    char *buf = new char[MAX_MSG_LEN];
    int recv_size = 0;
    while (true) {
        // 读取客户端的数据到buf中
        memset(buf, 0, sizeof(buf));
        if ((recv_size = read(conn_fd, buf, sizeof(buf))) == -1) {
            printf("read error:%s\n", strerror(errno));
            continue;
        }

        Request req;
        req.conn = this;
        req.data = buf;
        router.PreHandle(req);
        router.Handle(req);
        router.PostHandle(req);
//        int ret = handle_api(conn_fd, buf, recv_size);
//        if (ret != E_OK) {
//            printf("handle is error, conn_id:%d error_code:%d \n", conn_id, ret);
//            break;
//        }
    }
    delete buf;
    return 0;
}
