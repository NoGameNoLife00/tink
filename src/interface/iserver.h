//
// Created by admin on 2020/5/14.
//
#ifndef I_SERVER_H
#define I_SERVER_H

#include <memory>
#include "irouter.h"
namespace tink {
    class IServer {
    public:
        // ��ʼ��
//    IServer();
//    virtual int Init();
        // ����
        virtual int Start() = 0;
        // ֹͣ
        virtual int Stop() = 0;
        // ����
        virtual int Run() = 0;
        // ����ǰ����ע��һ��·�ɷ��������ͻ������Ӵ���ʹ��
        virtual int AddRouter(uint32_t msg_id, std::shared_ptr<IRouter> &router) = 0;

        virtual void OperateEvent(int fd, int op, int state) {};
//    virtual ~IServer() = 0;
    };
}


#endif