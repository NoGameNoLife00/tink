//
// Created by admin on 2020/5/20.
//

#ifndef TINK_CONNECTION_H
#define TINK_CONNECTION_H

#include <iconnection.h>
#include <error_code.h>
#include <irouter.h>
#include <memory>
#include <imsg_handler.h>

namespace tink {



class Connection : public IConnection, public std::enable_shared_from_this<Connection> {
    public:
        int Init(int conn_fd, int id, std::shared_ptr<IMsgHandler> &msg_handler);

        int Start();
        // ֹͣ����
        int Stop();
        // ��ȡ���ӵ�socket
        int GetTcpConn();
        // ��ȡ����id
        int GetConnId();

        int StartReader();
        // ��ȡ�ͻ��˵�tcp״̬ ip port
        RemoteAddrPtr GetRemoteAddr();
//        // �������ݵ��ͻ���
//        int Send(char *buf, int len);
        // ����Msg�����ͻ���
    int SendMsg(uint msg_id, std::shared_ptr<byte> &data, uint data_len);
    private:
        int conn_fd_;
        int conn_id_;
        bool is_close_;
//    conn_handle_func handle_api;
        RemoteAddrPtr remote_addr_;
        // ��Ϣ������
        std::shared_ptr<IMsgHandler> msg_handler_;
//        std::shared_ptr<IRouter> router;
    };
}



#endif //TINK_CONNECTION_H
