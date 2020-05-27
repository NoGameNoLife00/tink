//
// Created by admin on 2020/5/14.
//
#ifndef SERVER_H
#define SERVER_H

#include <iserver.h>
#include <string>
//typedef struct ServerInfo_ {
//
//}ServerInfo;

#define NAME_STR_LEN 64
#define IP_STR_LEN 20
#define MAX_MSG_LEN 2048
class Server : public IServer {
public:
    char name[NAME_STR_LEN];
    int ip_version;
    char ip[IP_STR_LEN];
    int port;
    IRouter *router;
    int Init(char *name, int ip_version, char *ip, int port);
    int Start();
    int Run();
    int Stop();

    int AddRouter(IRouter *router);
};
#endif