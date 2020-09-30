#ifndef TINK_CONNECTION_H
#define TINK_CONNECTION_H

#include <error_code.h>
#include <base_router.h>
#include <memory>
#include <message_queue.h>
#include <message.h>
#include <atomic>
#include <buffer.h>
#include <socket.h>
#include <map>
#include <server.h>
#include <message_handler.h>

namespace tink {
    class Server;
    class MessageHandler;
    typedef std::shared_ptr<Server> ServerPtr;
    typedef std::shared_ptr<MessageHandler> MessageHandlerPtr;

    class Connection : public std::enable_shared_from_this<Connection>
        {
    public:
        int Init(ServerPtr &&server, int conn_fd, int id, MessageHandlerPtr &msg_handler, SockAddressPtr &addr);

        int Start();
        // 停止链接
        int Stop();
        // 获取链接的socket
        int GetTcpConn() { return socket_->GetSockFd(); }
        // 获取链接id
        int GetConnId() { return conn_id_; }

        FixBufferPtr& GetBuffer() {return buffer_;}

        const MessageHandlerPtr &GetMsgHandler() { return msg_handler_;}

        // 获取客户端的tcp状态 ip port
        SockAddressPtr GetRemoteAddr() { return remote_addr_; }

        // 发送Msg包到写线程
        int SendMsg(uint32_t msg_id, UBytePtr &data, uint32_t data_len);

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
        MessageHandlerPtr msg_handler_;
        // 所属server
        ServerPtr server;
        FixBufferPtr buffer_;
        mutable Mutex mutex_;
        UBytePtr tmp_buffer_;
        uint32_t tmp_buffer_size_;
        std::map<int, std::string> property_;
    };

    typedef std::shared_ptr<Connection> ConnectionPtr;
}



#endif //TINK_CONNECTION_H
