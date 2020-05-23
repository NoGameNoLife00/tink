//
// Created by admin on 2020/5/19.
//

#ifndef INTERFACE_CONN_H
#define INTERFACE_CONN_H

#include <cygwin/socket.h>

typedef int (*conn_handle_func)(int, char*, int);

class interface_conn {
    // 启动链接
    int start();
    // 停止链接
    int stop();
    // 获取链接的socket
    int get_tcp_conn();
    // 获取链接id
    int get_conn_id();
    // 获取客户端的tcp状态 ip port
    sockaddr* get_remote_addr();
    // 发送数据到客户端
    int send(char *buf, int len);
};


#endif //TINK_INTERFACE_CONN_H
