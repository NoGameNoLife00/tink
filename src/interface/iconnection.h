//
// Created by admin on 2020/5/19.
//

#ifndef INTERFACE_CONN_H
#define INTERFACE_CONN_H

#include <cygwin/socket.h>

namespace tink {
//    typedef int (*conn_handle_func)(int, char*, int);

    class IConnection {
    public:
        // 启动链接
        virtual int Start() = 0;
        // 停止链接
        virtual int Stop() = 0;

        // 获取链接id
        virtual int GetConnId() {
            return 0;
        };
        // 获取客户端的tcp状态 ip port
        virtual struct sockaddr* GetRemoteAddr() {
            return 0;
        };
        // 发送数据到客户端
        virtual int Send(char *buf, int len) {
            return 0;
        };
        // 获取链接的socket
        virtual int GetTcpConn() {
            return 0;
        };
    };

}


#endif //TINK_INTERFACE_CONN_H
