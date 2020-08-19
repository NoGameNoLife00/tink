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

namespace tink {
    typedef MessageQueue<IMessagePtr> IMessageQueue;

    class Connection : public IConnection
            , public std::enable_shared_from_this<Connection>
        {
    public:
        int Init(IServerPtr &&server, int conn_fd, int id, IMessageHandlerPtr &msg_handler, RemoteAddrPtr &addr);

        int Start();
        // ֹͣ����
        int Stop();
        // ��ȡ���ӵ�socket
        int GetTcpConn() {return conn_fd_;};
        // ��ȡ����id
        int GetConnId() {return conn_id_;};

        BytePtr& GetBuffer() {return buffer_;};

        uint32_t GetBufferLen() {return buffer_size_;};

        uint32_t GetBuffOffset() {return buff_offset_;};

        void SetBuffOffset(uint32_t offset) {
            buff_offset_ = offset;
        };

        const IMessageHandlerPtr &GetMsgHandler() { return msg_handler_;};

        // ��ȡ�ͻ��˵�tcp״̬ ip port
        RemoteAddrPtr GetRemoteAddr() { return remote_addr_;};

        // ����Msg����д�߳�
        int SendMsg(uint32_t msg_id, BytePtr &data, uint32_t data_len);

        std::mutex &GetMutex() { return mutex_;};

        ~Connection();
    private:
        int conn_fd_;
        int conn_id_;
        std::atomic<bool> is_close_;
        RemoteAddrPtr remote_addr_;
        // ��Ϣ������
        IMessageHandlerPtr msg_handler_;
        // ����server
        IServerPtr server;

        uint32_t buffer_size_;
        BytePtr buffer_;
        uint32_t buff_offset_;
        std::mutex mutex_;
        BytePtr tmp_buffer_;
        uint32_t tmp_buffer_size_;
    };
    typedef std::shared_ptr<Connection> ConnectionPtr;
}



#endif //TINK_CONNECTION_H
