//
// Created by admin on 2020/5/19.
//

#ifndef INTERFACE_CONN_H
#define INTERFACE_CONN_H

#include <cygwin/socket.h>

typedef int (*conn_handle_func)(int, char*, int);

class interface_conn {
    // ��������
    int start();
    // ֹͣ����
    int stop();
    // ��ȡ���ӵ�socket
    int get_tcp_conn();
    // ��ȡ����id
    int get_conn_id();
    // ��ȡ�ͻ��˵�tcp״̬ ip port
    sockaddr* get_remote_addr();
    // �������ݵ��ͻ���
    int send(char *buf, int len);
};


#endif //TINK_INTERFACE_CONN_H
