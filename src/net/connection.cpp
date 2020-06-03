//
// Created by admin on 2020/5/20.
//

#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <connection.h>
#include <server.h>
#include <request.h>
#include <connection.h>
#include <global_mng.h>


namespace tink {

    int Connection::Init(int conn_fd, int id, std::shared_ptr<IRouter> router) {
        this->conn_fd = conn_fd;
        this->conn_id = id;
        this->router = router;
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

    RemoteAddrPtr Connection::GetRemoteAddr() {
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
        std::shared_ptr<GlobalMng> globalObj = tink::Singleton<tink::GlobalMng>::GetInstance();
        std::shared_ptr<byte> buf(new char[globalObj->GetMaxPackageSize()]);
        int recv_size = 0;
        while (true) {
            // 读取客户端的数据到buf中
            memset(buf.get(), 0, sizeof(buf));
            if ((recv_size = read(conn_fd, buf.get(), sizeof(buf.get()))) == -1) {
                printf("read error:%s\n", strerror(errno));
                continue;
            }

            Request req(*this, buf);
//            req.conn = std::shared_ptr<IConnection>(this);
            router->PreHandle(req);
            router->Handle(req);
            router->PostHandle(req);
//        int ret = handle_api(conn_fd, buf, recv_size);
//        if (ret != E_OK) {
//            printf("handle is error, conn_id:%d error_code:%d \n", conn_id, ret);
//            break;
//        }
        }
        return 0;
    }

}
