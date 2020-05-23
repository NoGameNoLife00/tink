//
// Created by admin on 2020/5/14.
//
#ifndef SERVER_H
#define SERVER_H

#include <interface_server.h>
#include <string>
//typedef struct ServerInfo_ {
//
//}ServerInfo;

#define NAME_STR_LEN 64
#define IP_STR_LEN 20
#define MAX_MSG_LEN 2048
class server : public interface_server {
public:
    char name[NAME_STR_LEN];
    int ip_version;
    char ip[IP_STR_LEN];
    int port;
    int init(char* name, int ip_version, char* ip, int port);
    int start();
    int run();
    int stop();
};
#endif