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
        // 初始化
//    IServer();
//    virtual int Init();
        // 启动
        virtual int Start() = 0;
        // 停止
        virtual int Stop() = 0;
        // 运行
        virtual int Run() = 0;
        // 给当前服务注册一个路由方法，供客户端链接处理使用
        virtual int AddRouter(uint32_t msg_id, std::shared_ptr<IRouter> &router) = 0;

        virtual void OperateEvent(int fd, int op, int state) {};
//    virtual ~IServer() = 0;
    };
}


#endif