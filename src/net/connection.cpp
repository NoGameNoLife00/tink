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
#include <arpa/inet.h>
#include "datapack.h"
#include "message.h"
#include <pthread.h>

namespace tink {
    IMessageQueue Connection::msg_queue;
    int Connection::Init(int conn_fd, int id, std::shared_ptr<IMessageHandler> &msg_handler) {
        this->conn_fd_ = conn_fd;
        this->conn_id_ = id;
        this->msg_handler_ = msg_handler;
        this->is_close_ = false;
        return 0;
    }

    int Connection::Stop() {
        logger->info("conn_ stop, conn_id:%v\n", conn_id_);
        if (is_close_) {
            return 0;
        }
        // 关闭读写线程
        is_close_ = true;
        // 回收socket
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
        logger->info("conn_ Start; conn_id:%v\n", conn_id_);
        // ?????????????????????
        if (pipe(fds_) < 0) {
            logger->info("read msg head error:%v\n", strerror(errno));
            return E_FAILED;
        }
        if (pthread_create(&writer_pid, NULL, StartWriter, this) != 0) {
            logger->info("create writer thread error:%v\n", strerror(errno));
            return E_FAILED;
        }
        if (pthread_create(&reader_pid, NULL, StartReader, this) != 0) {
            logger->info("create reader thread error:%v\n", strerror(errno));
            return E_FAILED;
        }
        return 0;
    }

    int Connection::SendMsg(uint32_t msg_id, std::shared_ptr<byte> &data, uint32_t data_len) {
        if (is_close_) {
            return E_CONN_CLOSED;
        }
        DataPack dp;
        std::shared_ptr<Message> msg = std::make_shared<Message>();
        byte *buf;
        uint32_t buf_len;
        msg->Init(msg_id, data_len, data);
        msg_queue.Push(msg);
        return 0;
    }

    void Connection::SetReaderPid(pid_t readerPid) {
        reader_pid = readerPid;
    }

    void *Connection::StartWriter(void *conn_ptr) {
        Connection *conn = static_cast<Connection*>(conn_ptr);
        if (!conn_ptr) {
            logger->info("[writer] thread run error, conn_ptr is null\n");
            return nullptr;
        }
        char *addr_str = inet_ntoa(((struct sockaddr_in*) conn->GetRemoteAddr().get())->sin_addr);
        int conn_id = conn->GetConnId();
        int conn_fd = conn->GetTcpConn();
        logger->info("[writer] thread is running, conn_id=%v, client addr=%v\n", conn->GetConnId(), addr_str);
        std::shared_ptr<GlobalMng> globalObj = GlobalInstance;
        int buf_size = globalObj->GetMaxPackageSize();
        byte *read_buf = new byte[buf_size];
        DataPack dp;
//        Message msg;
        int read_len = 0;
        while (true) {
//            memset(read_buf, 0, buf_size);
//            if ((read_len = read(fds_[0], read_buf, buf_size)) == -1) {
//                logger->info("[writer] read pipe error %v\n", strerror(errno));
//                break;
//            }
//            dp.Unpack(read_buf, msg);
            IMessagePtr msg;
            msg_queue.Pop(msg, true);
            if (msg->GetId() == -1) {
                logger->info("[writer] read exit msg: conn_id=%v", conn->GetConnId() );
                break;
            }

            if (send(conn_fd, read_buf, read_len, 0) == -1) {
                logger->info("[writer] send msg error %v\n", strerror(errno));
                break;
            }
        }

        logger->info("[writer] thread exit, conn_id=%v, client_addr=%v", conn_id, addr_str);
        // ???????
//        close(fds_[0]);
        return nullptr;
    }

    void *Connection::StartReader(void *conn_ptr) {
        Connection *conn = static_cast<Connection*>(conn_ptr);
        if (!conn_ptr) {
            logger->info("[reader] thread run error, conn_ptr is null\n");
            return nullptr;
        }

        logger->info("[reader] thread is running\n");
        std::shared_ptr<GlobalMng> globalObj = GlobalInstance;
        DataPack dp;
        std::shared_ptr<byte> head_data(new byte[dp.GetHeadLen()] {0});
        int conn_fd = conn->GetTcpConn();
        while (true) {
            // 读取客户端的数据到buf中
            std::shared_ptr<IMessage> msg(new Message);
            // 读取客户端发送的包头
            memset(head_data.get(), 0, dp.GetHeadLen());
            if ((read(conn_fd, head_data.get(), dp.GetHeadLen())) == -1) {
                logger->info("[reader] msg head error:%v\n", strerror(errno));
                break;
            }
            int e_code = dp.Unpack(head_data.get(), *msg.get());
            if (e_code != E_OK) {
                logger->info("[reader] unpack error: %v\n", e_code);
                break;
            }
            // 根据dataLen，再读取Data,放入msg中
            if (msg->GetDataLen() > 0) {
                std::shared_ptr<byte> buf(new byte[msg->GetDataLen()] {0});
                if ((read(conn_fd, buf.get(), msg->GetDataLen()) == -1)) {
                    logger->info("[reader] msg data error:%v\n", strerror(errno));
                    break;
                }
                msg->SetData(buf);
            }

            IRequestPtr req_ptr = std::make_shared<Request>(*conn, msg);
            if (globalObj->GetWorkerPoolSize() > 0) {
                conn->GetMsgHandler()->SendMsgToTaskQueue(req_ptr);
            } else {
//                msg_handler_->DoMsgHandle(req);
            }
//            req.conn_ = std::shared_ptr<IConnection>(this);
//            msg_handler_->DoMsgHandle(req);
//        int ret = handle_api(conn_fd, buf, recv_size);
//        if (ret != E_OK) {
//            logger->info("handle is error, conn_id:%v error_code:%v \n", conn_id, ret);
//            break;
//        }
        }
        // 发消息关闭写进程
//        byte *close_buf;
//        uint32_t close_msg_len;
        std::shared_ptr<Message> close_msg(new Message);
        close_msg->SetId(-1);
        close_msg->SetDataLen(0);
        msg_queue.Push(close_msg);
        return nullptr;
    }

    const std::shared_ptr<IMessageHandler> &Connection::GetMsgHandler() const {
        return msg_handler_;
    }

}
