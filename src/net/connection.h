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

        FixBufferPtr& GetBuffer() {return buffer_;};

        const IMessageHandlerPtr &GetMsgHandler() { return msg_handler_;};

        // ��ȡ�ͻ��˵�tcp״̬ ip port
        RemoteAddrPtr GetRemoteAddr() { return remote_addr_;};

        // ����Msg����д�߳�
        int SendMsg(uint32_t msg_id, BytePtr &data, uint32_t data_len);

        Mutex &GetMutex() { return mutex_;};

        ~Connection();
    private:
        static const int BUFFER_SIZE = 40960;
        int conn_fd_;
        int conn_id_;
        std::atomic_bool is_close_;
        RemoteAddrPtr remote_addr_;
        // ��Ϣ������
        IMessageHandlerPtr msg_handler_;
        // ����server
        IServerPtr server;

//        uint32_t buffer_size_;
//        BytePtr buffer_;
//        uint32_t buff_offset_;

        FixBufferPtr buffer_;
        mutable Mutex mutex_;
        BytePtr tmp_buffer_;
        uint32_t tmp_buffer_size_;
    };
    typedef std::shared_ptr<Connection> ConnectionPtr;
}



#endif //TINK_CONNECTION_H
