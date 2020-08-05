//
// Created by admin on 2020/5/14.
//
#ifndef SERVER_H
#define SERVER_H

#include <iserver.h>
#include <string>
#include <memory>
#include <imessage_handler.h>

namespace tink {

    class Server : public IServer {
    public:
        int Init(std::shared_ptr<std::string> name, int ip_version,
                 std::shared_ptr<std::string> ip, int port,
                 std::shared_ptr<IMessageHandler> &&msg_handler);
        int Start();
        int Run();
        int Stop();
        int AddRouter(uint32_t msg_id, std::shared_ptr<IRouter> &router);
    private:
        std::shared_ptr<std::string> name_;
        std::shared_ptr<std::string> ip_;
        int ip_version_;
        int port_;
        std::shared_ptr<IMessageHandler> msg_handler_;
    };
}


#endif