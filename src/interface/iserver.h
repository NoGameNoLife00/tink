//
// Created by admin on 2020/5/14.
//
#ifndef I_SERVER_H
#define I_SERVER_H

#include <memory>
#include <functional>
#include "irouter.h"
#include "iconn_manager.h"

namespace tink {
    typedef std::function<void(IConnectionPtr&)> ConnHookFunc;
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
        virtual IConnManagerPtr& GetConnMng() =0;
        virtual ~IServer() {};
        virtual void SetOnConnStart(ConnHookFunc &&func) {};
        virtual void SetOnConnStop(ConnHookFunc &&func) {};
        virtual void CallOnConnStart(IConnectionPtr &&conn) {};
        virtual void CallOnConnStop(IConnectionPtr &&conn) {};
    };
    typedef std::shared_ptr<IServer> IServerPtr;
}


#endif