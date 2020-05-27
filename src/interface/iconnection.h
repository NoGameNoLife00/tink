//
// Created by admin on 2020/5/19.
//

#ifndef INTERFACE_CONN_H
#define INTERFACE_CONN_H

#include <cygwin/socket.h>

typedef int (*conn_handle_func)(int, char*, int);

class IConnection {
    // 启动链接
    int Start();
    // 停止链接
    int Stop();
    // 获取链接的socket
    int GetTcpConn();
    // 获取链接id
    int GetConnId();
    // 获取客户端的tcp状态 ip port
    sockaddr* GetRemoteAddr();
    // 发送数据到客户端
    int Send(char *buf, int len);
};


#endif //TINK_INTERFACE_CONN_H
