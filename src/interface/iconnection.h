//
// Created by admin on 2020/5/19.
//

#ifndef INTERFACE_CONN_H
#define INTERFACE_CONN_H

#include <cygwin/socket.h>

typedef int (*conn_handle_func)(int, char*, int);

class IConnection {
    // ��������
    int Start();
    // ֹͣ����
    int Stop();
    // ��ȡ���ӵ�socket
    int GetTcpConn();
    // ��ȡ����id
    int GetConnId();
    // ��ȡ�ͻ��˵�tcp״̬ ip port
    sockaddr* GetRemoteAddr();
    // �������ݵ��ͻ���
    int Send(char *buf, int len);
};


#endif //TINK_INTERFACE_CONN_H
