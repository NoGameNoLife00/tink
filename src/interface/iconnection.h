//
// Created by admin on 2020/5/19.
//

#ifndef INTERFACE_CONN_H
#define INTERFACE_CONN_H

#include <sys/socket.h>
#include <memory>
#include <type.h>

namespace tink {
//    typedef int (*conn_handle_func)(int, char*, int);
    typedef  std::shared_ptr<struct sockaddr> RemoteAddrPtr;

    class IConnection {
    public:
        // ��������
        virtual int Start() = 0;
        // ֹͣ����
        virtual int Stop() = 0;

        // ��ȡ����id
        virtual int GetConnId() {
            return 0;
        };
        // ��ȡ�ͻ��˵�tcp״̬ ip port
        virtual RemoteAddrPtr GetRemoteAddr() = 0;
        // ����msg�����ͻ���
        virtual int SendMsg(uint32_t msg_id, BytePtr &data, uint32_t data_len) {
            return 0;
        };
        // ��ȡ���ӵ�socket
        virtual int GetTcpConn() {
            return 0;
        };
        virtual ~IConnection(){};
    };

    typedef std::shared_ptr<IConnection> IConnectionPtr;
}


#endif //TINK_INTERFACE_CONN_H
