//
// Created by admin on 2020/5/14.
//
#ifndef SERVER_H
#define SERVER_H

#include <iserver.h>
#include <string>
#include <memory>
#include <imsg_handler.h>

#define NAME_STR_LEN 64
#define IP_STR_LEN 20
#define MAX_MSG_LEN 2048
namespace tink {

    class Server : public IServer {
    public:
//        IRouter *router;
        int Init(std::shared_ptr<std::string> name, int ip_version,
                 std::shared_ptr<std::string> ip, int port,
                 std::shared_ptr<IMsgHandler> &msg_handler);
        int Start();
        int Run();
        int Stop();

        int AddRouter(uint msg_id, std::shared_ptr<IRouter> &router);

    private:
        std::shared_ptr<std::string> name;
        std::shared_ptr<std::string> ip;
        int ip_version;
        int port;
        std::shared_ptr<IRouter> router;
        std::shared_ptr<IMsgHandler> msg_handler_;
    };
}


#endif