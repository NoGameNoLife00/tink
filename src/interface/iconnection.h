//
// Created by admin on 2020/5/19.
//

#ifndef INTERFACE_CONN_H
#define INTERFACE_CONN_H

#include <cygwin/socket.h>

namespace tink {
//    typedef int (*conn_handle_func)(int, char*, int);

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
        virtual struct sockaddr* GetRemoteAddr() {
            return 0;
        };
        // �������ݵ��ͻ���
        virtual int Send(char *buf, int len) {
            return 0;
        };
        // ��ȡ���ӵ�socket
        virtual int GetTcpConn() {
            return 0;
        };
    };

}


#endif //TINK_INTERFACE_CONN_H
