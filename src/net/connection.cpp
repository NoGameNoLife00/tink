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
#include "datapack.h"
#include "message.h"


namespace tink {

    int Connection::Init(int conn_fd, int id, std::shared_ptr<IMsgHandler> &msg_handler) {
        this->conn_fd_ = conn_fd;
        this->conn_id_ = id;
        this->msg_handler_ = msg_handler;
        this->is_close_ = false;
        return 0;
    }

    int Connection::Stop() {
        printf("conn Stop, conn_id:%d\n", conn_id_);
        if (is_close_) {
            return 0;
        }

        is_close_ = true;
        // 回收资源
        close(conn_fd_);
        return E_OK;
    }

    int Connection::GetTcpConn() {
        return this->conn_fd_;
    }

    int Connection::GetConnId() {
        return  this->conn_id_;
    }

    RemoteAddrPtr Connection::GetRemoteAddr() {
        return this->remote_addr_;
    }

    int Connection::Start() {
        printf("conn Start; conn_id:%d\n", conn_id_);
        // 启动当前链接读取数据的业务
//        int pid = fork();
//        if (pid == 0) {
//            StartReader();
//        }
        StartReader();
        return 0;
    }

    int Connection::StartReader() {
        printf("reader process is running\n");

        std::shared_ptr<GlobalMng> globalObj = tink::Singleton<tink::GlobalMng>::GetInstance();
        DataPack dp;
        std::shared_ptr<byte> head_data(new byte[dp.GetHeadLen()] {0});
        while (true) {
            // 读取客户端的数据到buf中
            std::shared_ptr<IMessage> msg(new Message);
            // 读取客户端发送的包头
            memset(head_data.get(), 0, dp.GetHeadLen());
            if ((read(conn_fd_, head_data.get(), dp.GetHeadLen())) == -1) {
                printf("read msg head error:%s\n", strerror(errno));
                break;
            }
            int e_code = dp.Unpack(head_data.get(), *msg.get());
            if (e_code != E_OK) {
                printf("unpack error: %d\n", e_code);
                break;
            }
            // 根据dataLen，再读取Data,放入msg中
            if (msg->GetDataLen() > 0) {
                std::shared_ptr<byte> buf(new byte[msg->GetDataLen()] {0});
                if ((read(conn_fd_, buf.get(), msg->GetDataLen()) == -1)) {
                    printf("read msg data error:%s\n", strerror(errno));
                    break;
                }
                msg->SetData(buf);
            }

            std::shared_ptr<IConnection> conn = shared_from_this();
            Request req(conn, msg);
//            req.conn = std::shared_ptr<IConnection>(this);
            msg_handler_->DoMsgHandle(req);
//        int ret = handle_api(conn_fd, buf, recv_size);
//        if (ret != E_OK) {
//            printf("handle is error, conn_id:%d error_code:%d \n", conn_id, ret);
//            break;
//        }
        }
        return 0;
    }

    int Connection::SendMsg(uint msg_id, std::shared_ptr<byte> &data, uint data_len) {
        if (is_close_) {
            return E_CONN_CLOSED;
        }
        DataPack dp;
        Message msg;
        byte *buf;
        uint buf_len;
        msg.Init(msg_id, data_len, data);
        // data封包成二进制数据
        if (dp.Pack(msg, &buf, &buf_len) != E_OK) {
            printf("pack error msg id = %d\n", msg_id);
            return E_PACK_FAILED;
        }
        // 发送序封好的二进制数据
        if (send(conn_fd_, buf, buf_len, 0) == -1) {
            printf("send msg error %s\n", strerror(errno));
            return E_FAILED;
        }
        delete [] buf;
        return 0;
    }

}
