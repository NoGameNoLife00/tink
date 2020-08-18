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
    int Connection::Init(IServerPtr &&server, int conn_fd, int id, IMessageHandlerPtr &msg_handler, RemoteAddrPtr &addr) {
        this->server = server;
        this->conn_fd_ = conn_fd;
        this->conn_id_ = id;
        this->msg_handler_ = msg_handler;
        this->is_close_ = false;
        this->remote_addr_ = addr;
        this->server->GetConnMng()->Add(std::dynamic_pointer_cast<IConnection>(shared_from_this()));
        return 0;
    }

    int Connection::Stop() {
        logger->info("conn_ stop, conn_id:%v\n", conn_id_);

        if (is_close_) {
            return 0;
        }
        server->CallOnConnStop(std::dynamic_pointer_cast<IConnection>(shared_from_this()));
        // 关闭读写线程
        is_close_ = true;

        server->GetConnMng()->Remove(std::dynamic_pointer_cast<IConnection>(shared_from_this()));

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
        // 开启读写线程
        if (pthread_create(&writer_pid, NULL, StartWriter, new ConnectionPtr(shared_from_this())) != 0) {
            logger->error("create writer thread error:%v\n", strerror(errno));
            return E_FAILED;
        }
        if (pthread_create(&reader_pid, NULL, StartReader, new ConnectionPtr(shared_from_this())) != 0) {
            logger->error("create reader thread error:%v\n", strerror(errno));
            return E_FAILED;
        }
        server->CallOnConnStart(std::dynamic_pointer_cast<IConnection>(shared_from_this()));
        return 0;
    }

    int Connection::SendMsg(uint32_t msg_id, BytePtr &data, uint32_t data_len) {
        if (is_close_) {
            return E_CONN_CLOSED;
        }
        std::shared_ptr<Message> msg = std::make_shared<Message>();
        msg->Init(msg_id, data_len, data);
        msg_queue.Push(msg);
        return 0;
    }

    void Connection::SetReaderPid(pid_t readerPid) {
        reader_pid = readerPid;
    }

    void *Connection::StartWriter(void *ptr) {
        ConnectionPtr conn = *static_cast<ConnectionPtr*>(ptr);
        delete static_cast<ConnectionPtr*>(ptr);
//        Connection *conn = static_cast<Connection*>(conn_ptr);
        if (!conn) {
            logger->error("[writer] thread run error, conn_ptr is null\n");
            return nullptr;
        }
        char *addr_str = inet_ntoa(((struct sockaddr_in*) conn->GetRemoteAddr().get())->sin_addr);
        int conn_id = conn->GetConnId();
        int conn_fd = conn->GetTcpConn();
        logger->info("[writer] thread is running, conn_id=%v, client addr=%v\n", conn->GetConnId(), addr_str);
        DataPack dp;
        while (true) {
            byte *read_buf;
            uint32_t buf_size;
            IMessagePtr msg;
            msg_queue.Pop(msg, true);
            dp.Pack(*msg, &read_buf, &buf_size);

            if (msg->GetId() == -1) {
                logger->warn("[writer] read exit msg: conn_id=%v", conn->GetConnId() );
                break;
            }

            if (send(conn_fd, read_buf, buf_size, 0) == -1) {
                logger->error("[writer] send msg error %v\n", strerror(errno));
                break;
            }
        }

        logger->info("[writer] thread exit, conn_id=%v, client_addr=%v", conn_id, addr_str);
        return nullptr;
    }

    void *Connection::StartReader(void *ptr) {
        ConnectionPtr conn = *static_cast<ConnectionPtr*>(ptr);
        delete static_cast<ConnectionPtr*>(ptr);
        if (!conn) {
            logger->info("[reader] thread run error, conn_ptr is null\n");
            return nullptr;
        }

        logger->info("[reader] thread is running\n");
        DataPack dp;
        std::shared_ptr<byte> head_data(new byte[dp.GetHeadLen()] {0});
        int conn_fd = conn->GetTcpConn();
        while (true) {
            // 读取客户端的数据到buf中
            std::shared_ptr<IMessage> msg(new Message);
            // 读取客户端发送的包头
            memset(head_data.get(), 0, dp.GetHeadLen());
            if ((read(conn_fd, head_data.get(), dp.GetHeadLen())) == -1) {
                logger->warn("[reader] msg head error:%v\n", strerror(errno));
                break;
            }
            int e_code = dp.Unpack(head_data.get(), *msg.get());
            if (e_code != E_OK) {
                logger->warn("[reader] unpack error: %v\n", e_code);
                break;
            }
            // 根据dataLen，再读取Data,放入msg中
            if (msg->GetDataLen() > 0) {
                std::shared_ptr<byte> buf(new byte[msg->GetDataLen()] {0});
                if ((read(conn_fd, buf.get(), msg->GetDataLen()) == -1)) {
                    logger->warn("[reader] msg data error:%v\n", strerror(errno));
                    break;
                }
                msg->SetData(buf);
            }

            IRequestPtr req_ptr = std::make_shared<Request>(conn, msg);
            if (GlobalInstance->GetWorkerPoolSize() > 0) {
                conn->GetMsgHandler()->SendMsgToTaskQueue(req_ptr);
            } else {
//                msg_handler_->DoMsgHandle(req);
            }
        }
        IMessagePtr close_msg(new Message);
        close_msg->SetId(-1);
        close_msg->SetDataLen(0);
        msg_queue.Push(close_msg);
        return nullptr;
    }

    const IMessageHandlerPtr &Connection::GetMsgHandler() const {
        return msg_handler_;
    }

}
