//
// Created by admin on 2020/5/20.
//

#ifndef TINK_CONNECTION_H
#define TINK_CONNECTION_H

#include <interface_conn.h>
#include <error_code.h>
class connection : public interface_conn {
    int conn_fd;
    int conn_id;
    bool is_close;
    conn_handle_func handle_api;
    struct sockaddr remote_addr;
    int init(int conn_fd, int id, conn_handle_func callback);

    int start();
    // ֹͣ����
    int stop();
    // ��ȡ���ӵ�socket
    int get_tcp_conn();
    // ��ȡ����id
    int get_conn_id();

    int start_reader();
    // ��ȡ�ͻ��˵�tcp״̬ ip port
    sockaddr get_remote_addr();
    // �������ݵ��ͻ���
    int send(char *buf, int len);
};


#endif //TINK_CONNECTION_H
