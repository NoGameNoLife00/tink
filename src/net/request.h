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
        // ��ȡ��ǰ����
        ConnectionPtr & GetConnection();
        // ��ȡ�������Ϣ����
        UBytePtr& GetData();
        // ��Ϣ����
        uint32_t GetDataLen();
        // ��ȡ������Ϣ��ID
        int32_t GetMsgId();
        ~Request();
    private:
        ConnectionPtr conn_;
        MessagePtr msg_;
    };


}


#endif //TINK_REQUEST_H
