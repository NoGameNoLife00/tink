//
// Created by admin on 2020/5/14.
//
#ifndef I_SERVER_H
#define I_SERVER_H
class interface_server {
public:
    // 初始化
//    interface_server();
//    virtual int init();
    // 启动
    virtual int start() = 0;
    // 停止
    virtual int stop() {
        return 0;
    };
    // 运行
    virtual int run() {
        return 0;
    };

//    virtual ~interface_server() = 0;
};

#endif