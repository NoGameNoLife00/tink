//
// Created by admin on 2020/5/14.
//
#ifndef I_SERVER_H
#define I_SERVER_H
class interface_server {
public:
    // ��ʼ��
//    interface_server();
//    virtual int init();
    // ����
    virtual int start() = 0;
    // ֹͣ
    virtual int stop() {
        return 0;
    };
    // ����
    virtual int run() {
        return 0;
    };

//    virtual ~interface_server() = 0;
};

#endif