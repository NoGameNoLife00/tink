#ifndef TINK_CONNECTION_H
#define TINK_CONNECTION_H

#include <iconnection.h>
#include <error_code.h>
#include <irouter.h>
#include <memory>
#include <imessage_handler.h>
#include <message_queue.h>
#include <imessage.h>
#include <iserver.h>
#include <atomic>
#include <buffer.h>
#include <socket.h>
#include <map>

namespace tink {
    typedef MessageQueue<IMessagePtr> IMessageQueue;

    class Connection : public IConnection
            , public std::enable_shared_from_this<Connection>
        {
    public:
        int Init(IServerPtr &&server, int conn_fd, int id, IMessageHandlerPtr &msg_handler, SockAddressPtr &addr);

        int Start();
        // 停止链接
        int Stop();
        // 获取链接的socket
        int GetTcpConn() { return socket_->GetSockFd(); }
        // 获取链接id
        int GetConnId() { return conn_id_; }

        FixBufferPtr& GetBuffer() {return buffer_;}

        const IMessageHandlerPtr &GetMsgHandler() { return msg_handler_;}

        // 获取客户端的tcp状态 ip port
        SockAddressPtr GetRemoteAddr() { return remote_addr_; }

        // 发送Msg包到写线程
        int SendMsg(uint32_t msg_id, BytePtr &data, uint32_t data_len);

        Mutex &GetMutex() { return mutex_; }

        ~Connection();

        string GetProperty(int key);

        void SetProperty(int key, const string &val) ;

    private:
        int conn_id_;
        std::unique_ptr<Socket> socket_;
        std::atomic_bool is_close_;
        SockAddressPtr remote_addr_;
        // 消息管理器
        IMessageHandlerPtr msg_handler_;
        // 所属server
        IServerPtr server;
        FixBufferPtr buffer_;
        mutable Mutex mutex_;
        BytePtr tmp_buffer_;
        uint32_t tmp_buffer_size_;
        std::map<int, std::string> property_;
    };
    typedef std::shared_ptr<Connection> ConnectionPtr;
}



#endif //TINK_CONNECTION_H
