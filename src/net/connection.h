#ifndef TINK_CONNECTION_H
#define TINK_CONNECTION_H

#include <iconnection.h>
#include <error_code.h>
#include <irouter.h>
#include <memory>
#include <imessage_handler.h>
#include <message_queue.h>
#include <imessage.h>
#include <atomic>

namespace tink {
    typedef MessageQueue<IMessagePtr> IMessageQueue;

    class Connection : public IConnection
            , public std::enable_shared_from_this<Connection>
        {
    public:
        static void* StartWriter(void* conn_ptr);
        static void* StartReader(void* conn_ptr);

        int Init(int conn_fd, int id, IMessageHandlerPtr &msg_handler, RemoteAddrPtr &addr);

        int Start();
        // ֹͣ����
        int Stop();
        // ��ȡ���ӵ�socket
        int GetTcpConn();
        // ��ȡ����id
        int GetConnId();

        BytePtr& GetBuffer();

        uint32_t GetBufferLen();

        void SetReaderPid(pid_t readerPid);

        const IMessageHandlerPtr &GetMsgHandler();

        // ��ȡ�ͻ��˵�tcp״̬ ip port
        RemoteAddrPtr GetRemoteAddr();

        // ����Msg����д�߳�
        int SendMsg(uint32_t msg_id, BytePtr &data, uint32_t data_len);

        std::mutex &GetMutex();

    private:
        static IMessageQueue msg_queue;
        pthread_t writer_pid;
        pthread_t reader_pid;
        int conn_fd_;
        int conn_id_;
        std::atomic<bool> is_close_;
        RemoteAddrPtr remote_addr_;
        // ��Ϣ������
        IMessageHandlerPtr msg_handler_;

        uint32_t buffer_size_;
        BytePtr buffer_;
        std::mutex mutex_;
    };
    typedef std::shared_ptr<Connection> ConnectionPtr;
}



#endif //TINK_CONNECTION_H
