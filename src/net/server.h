#ifndef SERVER_H
#define SERVER_H

#include <iserver.h>
#include <string>
#include <memory>
#include <imessage_handler.h>
#include <iconn_manager.h>

namespace tink {



    class Server : public IServer
    , public std::enable_shared_from_this<Server>
            {
    public:
        int Init(StringPtr &name, int ip_version,
                 StringPtr &ip, int port,
                 IMessageHandlerPtr &&msg_handler);
        int Start();
        int Run();
        int Stop();
        int AddRouter(uint32_t msg_id, std::shared_ptr<IRouter> &router);
        IConnManagerPtr& GetConnMng() {return conn_mng_;};
        void SetOnConnStart(ConnHookFunc &&func);
        void SetOnConnStop(ConnHookFunc &&func);
        void CallOnConnStart(IConnectionPtr &&conn);
        void CallOnConnStop(IConnectionPtr &&conn);
    private:
        StringPtr name_;
        StringPtr ip_;
        int ip_version_;
        int port_;
        // server的消息管理模块
        IMessageHandlerPtr msg_handler_;
        // server的连接管理器
        IConnManagerPtr conn_mng_;

        // 连接启动和停止的钩子函数
        ConnHookFunc on_conn_start_;
        ConnHookFunc on_conn_stop_;
    };
}


#endif