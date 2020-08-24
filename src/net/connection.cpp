//
// Created by admin on 2020/5/20.
//

#include <unistd.h>
#include <cstring>
#include <connection.h>
#include <server.h>
#include <request.h>
#include <global_mng.h>
#include <arpa/inet.h>
#include "datapack.h"
#include "message.h"

#define BUFF_MAX_SIZE_COUNt 16
namespace tink {
    int Connection::Init(IServerPtr &&server, int conn_fd, int id, IMessageHandlerPtr &msg_handler, RemoteAddrPtr &addr) {
        int package_size = (GlobalInstance->GetMaxPackageSize() + DataPack::GetHeadLen());
        this->server = server;
        this->conn_fd_ = conn_fd;
        this->conn_id_ = id;
        this->msg_handler_ = msg_handler;
        this->is_close_ = false;
        this->remote_addr_ = addr;
        this->buffer_ = std::make_unique<FixBuffer>(package_size * BUFF_MAX_SIZE_COUNt);
        this->tmp_buffer_size_ = package_size;
        this->tmp_buffer_ = std::make_unique<byte[]>(tmp_buffer_size_);
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

    int Connection::Start() {
        logger->info("conn_ Start; conn_id:%v\n", conn_id_);
        server->CallOnConnStart(std::dynamic_pointer_cast<IConnection>(shared_from_this()));
        return 0;
    }

    int Connection::SendMsg(uint32_t msg_id, BytePtr &data, uint32_t data_len) {
        if (is_close_) {
            return E_CONN_CLOSED;
        }
        int ret = E_OK;
        std::lock_guard<Mutex> guard(mutex_);
        Message msg;
        msg.Init(msg_id, data_len, data);
        uint32_t len;
        memset(tmp_buffer_.get(), 0, tmp_buffer_size_);
        DataPack::Pack(msg, tmp_buffer_, len);
        ret = buffer_->Append(tmp_buffer_.get(), len);
        if (ret) {
            return ret;
        }
        GlobalInstance->GetServer()->OperateEvent(conn_fd_, conn_id_, EPOLL_CTL_MOD, EPOLLIN | EPOLLOUT);
        return E_OK;
    }

    Connection::~Connection() {
        logger->debug("conn %v is destruction", conn_id_);
    }
}
