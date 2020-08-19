//
// Created by admin on 2020/5/14.
//
#ifndef I_SERVER_H
#define I_SERVER_H

#include <memory>
#include "irouter.h"
#include "iconn_manager.h"

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


        virtual void OperateEvent(uint32_t fd, uint32_t id, int op, int state) {};

        virtual IConnManagerPtr& GetConnMng() =0;
        virtual ~IServer() {};
    };
    typedef std::shared_ptr<IServer> IServerPtr;
}


#endif