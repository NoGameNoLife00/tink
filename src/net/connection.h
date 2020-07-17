//
// Created by admin on 2020/5/20.
//

#ifndef TINK_CONNECTION_H
#define TINK_CONNECTION_H

#include <iconnection.h>
#include <error_code.h>
#include <irouter.h>
#include <memory>
#include <imessage_handler.h>

namespace tink {



class Connection : public IConnection
//        , public std::enable_shared_from_this<Connection>
        {
    public:
        static void* StartWriter(void* conn_ptr);
        static void* StartReader(void* conn_ptr);

        int Init(int conn_fd, int id, std::shared_ptr<IMessageHandler> &msg_handler);

        int Start();
        // 停止链接
        int Stop();
        // 获取链接的socket
        int GetTcpConn();
        // 获取链接id
        int GetConnId();
        // 开启读进程
        int StartReader();
        // 开启写进程
        int StartWriter();

        void SetReaderPid(pid_t readerPid);

        // 获取客户端的tcp状态 ip port
        RemoteAddrPtr GetRemoteAddr();
//        // 发送数据到客户端
//        int Send(char *buf, int len);
        // 发送Msg包到客户端
        int SendMsg(uint32_t msg_id, std::shared_ptr<byte> &data, uint32_t data_len);
    private:
        pthread_t writer_pid;
        pthread_t reader_pid;
        int conn_fd_;
        int conn_id_;
        bool is_close_;
        int fds_[2];
        RemoteAddrPtr remote_addr_;
        // 消息管理器
        std::shared_ptr<IMessageHandler> msg_handler_;
    };



}



#endif //TINK_CONNECTION_H
