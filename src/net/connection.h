//
// Created by admin on 2020/5/20.
//

#ifndef TINK_CONNECTION_H
#define TINK_CONNECTION_H

#include <iconnection.h>
#include <error_code.h>
#include <irouter.h>
#include <memory>
#include <imessage_handler.h>

namespace tink {



class Connection : public IConnection
//        , public std::enable_shared_from_this<Connection>
        {
    public:
        static void* StartWriter(void* conn_ptr);
        static void* StartReader(void* conn_ptr);

        int Init(int conn_fd, int id, std::shared_ptr<IMessageHandler> &msg_handler);

        int Start();
        // ֹͣ����
        int Stop();
        // ��ȡ���ӵ�socket
        int GetTcpConn();
        // ��ȡ����id
        int GetConnId();
        // ����������
        int StartReader();
        // ����д����
        int StartWriter();

        void SetReaderPid(pid_t readerPid);

        // ��ȡ�ͻ��˵�tcp״̬ ip port
        RemoteAddrPtr GetRemoteAddr();
//        // �������ݵ��ͻ���
//        int Send(char *buf, int len);
        // ����Msg�����ͻ���
        int SendMsg(uint32_t msg_id, std::shared_ptr<byte> &data, uint32_t data_len);
    private:
        pthread_t writer_pid;
        pthread_t reader_pid;
        int conn_fd_;
        int conn_id_;
        bool is_close_;
        int fds_[2];
        RemoteAddrPtr remote_addr_;
        // ��Ϣ������
        std::shared_ptr<IMessageHandler> msg_handler_;
    };



}



#endif //TINK_CONNECTION_H
