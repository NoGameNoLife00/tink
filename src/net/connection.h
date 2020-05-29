//
// Created by admin on 2020/5/20.
//

#ifndef TINK_CONNECTION_H
#define TINK_CONNECTION_H

#include <iconnection.h>
#include <error_code.h>
#include <irouter.h>
namespace tink {
    class Connection : public IConnection {
    private:
        int conn_fd;
        int conn_id;
        bool is_close;
//    conn_handle_func handle_api;
        struct sockaddr *remote_addr;
        IRouter *router;
    public:
        int Init(int conn_fd, int id, IRouter *router);

        int Start();
        // 停止链接
        int Stop();
        // 获取链接的socket
        int GetTcpConn();
        // 获取链接id
        int GetConnId();

        int StartReader();
        // 获取客户端的tcp状态 ip port
        struct sockaddr* GetRemoteAddr();
        // 发送数据到客户端
        int Send(char *buf, int len);
    };
}



#endif //TINK_CONNECTION_H
