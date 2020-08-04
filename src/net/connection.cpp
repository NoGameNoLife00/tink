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
        printf("conn_ stop, conn_id:%d\n", conn_id_);
        if (is_close_) {
            return 0;
        }
        // ????��????
//        kill(writer_pid ,SIGABRT);
//        kill(reader_pid, SIGABRT);
        is_close_ = true;
        // ????socket
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
        printf("conn_ Start; conn_id:%d\n", conn_id_);
        // ?????????????????????
        if (pipe(fds_) < 0) {
            printf("read msg head error:%s\n", strerror(errno));
            return E_FAILED;
        }
        if (pthread_create(&writer_pid, NULL, StartWriter, this) != 0) {
            printf("create writer thread error:%s\n", strerror(errno));
            return E_FAILED;
        }
        if (pthread_create(&reader_pid, NULL, StartReader, this) != 0) {
            printf("create reader thread error:%s\n", strerror(errno));
            return E_FAILED;
        }
        return 0;
    }

//    int Connection::StartReader() {
//        printf("[reader] thread is running\n");
//
//        std::shared_ptr<GlobalMng> globalObj = GlobalInstance;
//        DataPack dp;
//        std::shared_ptr<byte> head_data(new byte[dp.GetHeadLen()] {0});
//        while (true) {
//            // ??????????????buf??
//            std::shared_ptr<IMessage> msg(new Message);
//            // ??????????????
//            memset(head_data.get(), 0, dp.GetHeadLen());
//            if ((read(conn_fd_, head_data.get(), dp.GetHeadLen())) == -1) {
//                printf("read msg head error:%s\n", strerror(errno));
//                break;
//            }
//            int e_code = dp.Unpack(head_data.get(), *msg.get());
//            if (e_code != E_OK) {
//                printf("unpack error: %d\n", e_code);
//                break;
//            }
//            // ????dataLen??????Data,????msg??
//            if (msg->GetDataLen() > 0) {
//                std::shared_ptr<byte> buf(new byte[msg->GetDataLen()] {0});
//                if ((read(conn_fd_, buf.get(), msg->GetDataLen()) == -1)) {
//                    printf("read msg data error:%s\n", strerror(errno));
//                    break;
//                }
//                msg->SetData(buf);
//            }
//            IRequestPtr req_ptr = std::make_shared<Request>(*this, msg);
//            if (globalObj->GetWorkerPoolSize() > 0) {
//                msg_handler_->SendMsgToTaskQueue(req_ptr);
//            } else {
////                msg_handler_->DoMsgHandle(req);
//            }
//
//
////            req.conn_ = std::shared_ptr<IConnection>(this);
////            msg_handler_->DoMsgHandle(req);
////        int ret = handle_api(conn_fd, buf, recv_size);
////        if (ret != E_OK) {
////            printf("handle is error, conn_id:%d error_code:%d \n", conn_id, ret);
////            break;
////        }
//        }
//        // ????????��????
//        byte *close_buf;
//        uint32_t close_msg_len;
//        Message close_msg;
//        close_msg.SetId(-1);
//        close_msg.SetDataLen(0);
//        dp.Pack(close_msg, &close_buf, &close_msg_len);
//        if (write(fds_[1], close_buf, close_msg_len) != close_msg_len) {
//            printf("[reader] write pipe msg error %s\n", strerror(errno));
//            delete [] close_buf;
//            return E_FAILED;
//        }
//        delete [] close_buf;
//        // ???��???
//        close(fds_[1]);
//        return 0;
//    }

    int Connection::SendMsg(uint32_t msg_id, std::shared_ptr<byte> &data, uint32_t data_len) {
        if (is_close_) {
            return E_CONN_CLOSED;
        }
        DataPack dp;
        std::shared_ptr<Message> msg = std::make_shared<Message>();
        byte *buf;
        uint32_t buf_len;
        msg->Init(msg_id, data_len, data);
        // data??????????????
//        if (dp.Pack(msg, &buf, &buf_len) != E_OK) {
//            printf("[reader] pack error msg id = %d\n", msg_id);
//            delete [] buf;
//            return E_PACK_FAILED;
//        }
//        // ??????????????????��????
//        if (write(fds_[1], buf, buf_len) != buf_len) {
//            printf("[reader] write pipe msg error %s\n", strerror(errno));
//            delete [] buf;
//            return E_FAILED;
//        }
        msg_queue.Push(msg);
//        delete [] buf;
        return 0;
    }

//    int Connection::StartWriter() {
//        char *addr_str = inet_ntoa(((struct sockaddr_in*) remote_addr_.get())->sin_addr);
//        printf("[writer] thread is running, conn_id=%d, client addr=%s\n", conn_id_, addr_str);
//        std::shared_ptr<GlobalMng> globalObj = GlobalInstance;
//        int buf_size = globalObj->GetMaxPackageSize();
//        byte *read_buf = new byte[buf_size];
//        DataPack dp;
//        Message msg;
//        int read_len = 0;
//        while (true) {
//            memset(read_buf, 0, buf_size);
//            if ((read_len = read(fds_[0], read_buf, buf_size)) == -1) {
//                printf("[writer] read pipe error %s\n", strerror(errno));
//                break;
//            }
//            dp.Unpack(read_buf, msg);
//            if (msg.GetId() == -1) {
//                printf("[writer] read exit msg: conn_id=%d", conn_id_ );
//                break;
//            }
//
//            if (send(conn_fd_, read_buf, read_len, 0) == -1) {
//                printf("[writer] send msg error %s\n", strerror(errno));
//                break;
//            }
//        }
//
//        printf("[writer] thread exit, conn_id=%d, client_addr=%s", conn_id_, addr_str);
//        // ???????
//        close(fds_[0]);
//        return 0;
//    }

    void Connection::SetReaderPid(pid_t readerPid) {
        reader_pid = readerPid;
    }

    void *Connection::StartWriter(void *conn_ptr) {
        Connection *conn = static_cast<Connection*>(conn_ptr);
        if (!conn_ptr) {
            printf("[writer] thread run error, conn_ptr is null\n");
            return nullptr;
        }
        char *addr_str = inet_ntoa(((struct sockaddr_in*) conn->GetRemoteAddr().get())->sin_addr);
        int conn_id = conn->GetConnId();
        int conn_fd = conn->GetTcpConn();
        printf("[writer] thread is running, conn_id=%d, client addr=%s\n", conn->GetConnId(), addr_str);
        std::shared_ptr<GlobalMng> globalObj = GlobalInstance;
        int buf_size = globalObj->GetMaxPackageSize();
        byte *read_buf = new byte[buf_size];
        DataPack dp;
//        Message msg;
        int read_len = 0;
        while (true) {
//            memset(read_buf, 0, buf_size);
//            if ((read_len = read(fds_[0], read_buf, buf_size)) == -1) {
//                printf("[writer] read pipe error %s\n", strerror(errno));
//                break;
//            }
//            dp.Unpack(read_buf, msg);
            IMessagePtr msg;
            msg_queue.Pop(msg, true);
            if (msg->GetId() == -1) {
                printf("[writer] read exit msg: conn_id=%d", conn->GetConnId() );
                break;
            }

            if (send(conn_fd, read_buf, read_len, 0) == -1) {
                printf("[writer] send msg error %s\n", strerror(errno));
                break;
            }
        }

        printf("[writer] thread exit, conn_id=%d, client_addr=%s", conn_id, addr_str);
        // ???????
//        close(fds_[0]);
        return nullptr;
    }

    void *Connection::StartReader(void *conn_ptr) {
        Connection *conn = static_cast<Connection*>(conn_ptr);
        if (!conn_ptr) {
            printf("[reader] thread run error, conn_ptr is null\n");
            return nullptr;
        }

        printf("[reader] thread is running\n");
        std::shared_ptr<GlobalMng> globalObj = GlobalInstance;
        DataPack dp;
        std::shared_ptr<byte> head_data(new byte[dp.GetHeadLen()] {0});
        int conn_fd = conn->GetTcpConn();
        while (true) {
            // ??????????????buf??
            std::shared_ptr<IMessage> msg(new Message);
            // ??????????????
            memset(head_data.get(), 0, dp.GetHeadLen());
            if ((read(conn_fd, head_data.get(), dp.GetHeadLen())) == -1) {
                printf("[reader] msg head error:%s\n", strerror(errno));
                break;
            }
            int e_code = dp.Unpack(head_data.get(), *msg.get());
            if (e_code != E_OK) {
                printf("[reader] unpack error: %d\n", e_code);
                break;
            }
            // ????dataLen??????Data,????msg??
            if (msg->GetDataLen() > 0) {
                std::shared_ptr<byte> buf(new byte[msg->GetDataLen()] {0});
                if ((read(conn_fd, buf.get(), msg->GetDataLen()) == -1)) {
                    printf("[reader] msg data error:%s\n", strerror(errno));
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
//            printf("handle is error, conn_id:%d error_code:%d \n", conn_id, ret);
//            break;
//        }
        }
        // ????????��????
//        byte *close_buf;
//        uint32_t close_msg_len;
        std::shared_ptr<Message> close_msg(new Message);
        close_msg->SetId(-1);
        close_msg->SetDataLen(0);
        msg_queue.Push(close_msg);
//        dp.Pack(close_msg, &close_buf, &close_msg_len);
//        if (write(fds_[1], close_buf, close_msg_len) != close_msg_len) {
//            printf("[reader] write pipe msg error %s\n", strerror(errno));
//            delete [] close_buf;
//            return nullptr;
//        }

//        delete [] close_buf;
        // ???��???
//        close(fds_[1]);
        return nullptr;
    }

    const std::shared_ptr<IMessageHandler> &Connection::GetMsgHandler() const {
        return msg_handler_;
    }

}
