//
// Created by admin on 2020/5/14.
//
#ifndef I_SERVER_H
#define I_SERVER_H

#include "irouter.h"

class IServer {
public:
    // 初始化
//    IServer();
//    virtual int Init();
    // 启动
    virtual int Start() = 0;
    // 停止
    virtual int Stop();
    // 运行
    virtual int Run();
    // 给当前服务注册一个路由方法，供客户端链接处理使用
    virtual int AddRouter(IRouter router);
//    virtual ~IServer() = 0;
};

#endif