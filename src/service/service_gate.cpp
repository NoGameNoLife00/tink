
#include <error_code.h>
#include <spdlog/spdlog.h>
#include <context_manage.h>
#include <socket_server.h>

#include "service_gate.h"

#define BACKLOG 128
namespace tink::Service {
    ServiceGate::ServiceGate() : listen_id(-1) {

    }

    int ServiceGate::Init(ContextPtr ctx, const std::string &param) {
        if (param.empty()) {
            return E_FAILED;
        }
        char header;
        char watchdog[param.size()];
        char binding[param.size()];
        int client_tag = 0, max = 0;
        int n = sscanf(param.c_str(), "%c %s %s %d %d", &header, watchdog, binding, &client_tag, &max);
        if (n < 4){
            spdlog::error("invalid gate param {}", n);
            return E_FAILED;
        }
        if (max <= 0) {
            spdlog::error("need max connection");
            return E_FAILED;
        }
        if (client_tag == 0) {
            client_tag = PTYPE_CLIENT;
        }
        if (watchdog[0] = '!') {
            this->watchdog = 0;
        } else {
            this->watchdog = ContextMngInstance.FindName(watchdog);
            if (this->watchdog == 0) {
                spdlog::error("Invalid watchdog {}", static_cast<char*>(watchdog));
                return E_FAILED;
            }
        }
        this->ctx = ctx;
//        for (int i = 0 ; i < max; i++) {
//            ConnectionPtr cn = std::make_shared<Connection>();
//            cn->id = -1;
//            conn.emplace_back(cn);
//        }

        max_connection = max;
        conn_pool = std::make_shared<ConnPool>(max, 0);
        this->client_tag = client_tag;
        this->header_size = header == 'S' ? 2 : 4;
        ctx->SetCallBack(CallBack_, this);
        return 0;
    }

    void ServiceGate::Release() {

    }

    int ServiceGate::StartListen(std::string &listen_addr) {
        char * port_str = const_cast<char *>(strrchr(listen_addr.c_str(), ':'));
        string host = "";
        int port;
        if (port_str == nullptr) {
            port = strtol(listen_addr.c_str(), nullptr, 10);
            if (port <= 0) {
                spdlog::error("invalid gate address {}", listen_addr);
                return E_FAILED;
            }
        } else {
            port = strtol(port_str + 1, nullptr, 10);
            if (port <= 0) {
                spdlog::error("invalid gate address {}", listen_addr);
                return E_FAILED;
            }
            port_str[0] = '\0';
            host = listen_addr;
        }
        listen_id = SOCKET_SERVER.Listen(ctx->Handle(), host, port, BACKLOG);
        if (listen_id < 0) {
            return E_FAILED;
        }
        SOCKET_SERVER.Start(ctx->Handle(), listen_id);
        return E_OK;
    }

    int
    ServiceGate::CallBack_(Context &ctx, void *ud, int type, int session, uint32_t source, DataPtr &msg, size_t sz) {
        ServiceGate *g = static_cast<ServiceGate *>(ud);
        switch (type) {
            case PTYPE_TEXT:
                g->Ctrl(msg, sz);
                break;
            case PTYPE_CLIENT:

        }
        return 0;
    }

    void ServiceGate::Ctrl(DataPtr &msg, int sz) {
        char tmp[sz+1];
        memcpy(tmp, msg.get(), sz);
        tmp[sz] = '\0';
        std::string command(tmp);
        StringList param_list;
        StringUtil::Split(command, ' ', param_list);
        if (param_list.size() < 2) {
            return;
        }
        if (param_list[0] == "kick") {
            int uid = std::stol(param_list[1]);
            if (conn.find(uid) != conn.end()) {
                SOCKET_SERVER.Close(ctx->Handle(), uid);
            }
            return ;
        }
        if (param_list[0] == "forward") {
            if (param_list.size() < 4) {
                return;
            }
            int id = std::stol(param_list[1]);
            uint32_t agent_handle = std::stoul(param_list[2], 0, 16);
            uint32_t client_handle = std::stoul(param_list[3], 0, 16);
            ForwardAgent_(id, agent_handle, client_handle);
            return ;
        }
        if (param_list[0] == "broker") {
            broker = ContextMngInstance.QueryName(param_list[1]);
            return;
        }
        if (param_list[0] == "start") {
            int uid = std::stol(param_list[1]);
            if (conn.find(uid) != conn.end()) {
                SOCKET_SERVER.Start(ctx->Handle(), uid);
            }
            return ;
        }
        if (param_list[0] == "close") {
            if (listen_id >= 0) {
                SOCKET_SERVER.Close(ctx->Handle(), listen_id);
                listen_id = -1;
            }
            return;
        }
        spdlog::error("[gate] unknown command : {}", command);
    }

    void ServiceGate::ForwardAgent_(int fd, uint32_t agent_addr, uint32_t client_addr) {
        auto it =conn.find(fd);
        if (it != conn.end()) {
            Connection* agent = it->second;
            agent->agent = agent_addr;
            agent->client = client_addr;
        }
    }
}
