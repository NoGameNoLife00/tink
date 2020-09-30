#ifndef TINK_REQUEST_H
#define TINK_REQUEST_H

#include <message.h>
#include <connection.h>
#include <server.h>
namespace tink {
    class NetMessage;
    class Connection;

    typedef std::unique_ptr<NetMessage> MessagePtr;
    typedef std::shared_ptr<Connection> ConnectionPtr;
    class Request {
    public:
        Request(ConnectionPtr &conn, MessagePtr& msg);
        // 获取当前连接
        ConnectionPtr & GetConnection();
        // 获取请求的消息数据
        UBytePtr& GetData();
        // 消息长度
        uint32_t GetDataLen();
        // 获取请求消息的ID
        int32_t GetMsgId();
        ~Request();
    private:
        ConnectionPtr conn_;
        MessagePtr msg_;
    };


}


#endif //TINK_REQUEST_H
