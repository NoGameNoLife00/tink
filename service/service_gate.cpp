#include <sstream>
#include "error_code.h"
#include "spdlog/spdlog.h"
#include "base/handle_manager.h"
#include "net/socket_server.h"
#include "service_gate.h"

#define BACKLOG 128

extern "C" {
tink::BaseModule *CreateModule() {
    return new tink::Service::ServiceGate();
}
};



namespace tink::Service {
    ServiceGate::ServiceGate() : listen_id_(-1) {
        name_ = "service_gate";
    }

    int ServiceGate::Init(ContextPtr ctx, std::string_view param) {
        if (param.empty()) {
            return E_FAILED;
        }
        InitLog();
        std::istringstream is(param.data());
        char header;
        string watchdog;
        string binding;
        int client_tag = 0, max = -1;
        is >> header >> watchdog >> binding >> client_tag >> max;

        if (max <= 0) {
            logger->error("need max connection");
            return E_FAILED;
        }
        if (client_tag == 0) {
            client_tag = PTYPE_CLIENT;
        }
        if (watchdog[0] == '!') {
            this->watchdog_ = 0;
        } else {
            this->watchdog_ = ctx_->GetServer()->GetHandlerMgr()->FindName(watchdog);
            if (this->watchdog_ == 0) {
                logger->error("Invalid watchdog {}", watchdog);
                return E_FAILED;
            }
        }
        this->ctx_ = ctx;
        max_connection_ = max;
        conn_pool_ = std::make_shared<ConnPool>(max, 0);
        msg_pool_ = std::make_shared<MessagePool>(100, 100);
        this->client_tag_ = client_tag;
        this->header_size_ = header == 'S' ? 2 : 4;
        ctx->SetCallBack(CallBack_, this);
        return StartListen(binding);
    }

    void ServiceGate::Release() {
        for (auto& it : conn_) {
            Connection *c = it.second;
            ctx_->GetServer()->GetSocketServer()->Close(ctx_->Handle(), c->id);
            conn_pool_->ReusePoolItem(c);
        }
        conn_.clear();
        conn_pool_->DeletePool();
        msg_pool_->DeletePool();
        conn_pool_.reset();
    }

    int ServiceGate::StartListen(std::string_view listen_addr) {
        int port_idx = listen_addr.find_last_of(':');
        int port;
        if (port_idx == std::string_view::npos) {
            port = strtol(listen_addr.data(), nullptr, 10);
            if (port <= 0) {
                logger->error("invalid gate address {}", listen_addr);
                return E_FAILED;
            }
        } else {
            port = strtol(listen_addr.data() + port_idx + 1, nullptr, 10);
            if (port <= 0) {
                logger->error("invalid gate address {}", listen_addr);
                return E_FAILED;
            }

        }
        string host(listen_addr.substr(0, port_idx));
        listen_id_ = ctx_->GetServer()->GetSocketServer()->Listen(ctx_->Handle(), host, port, BACKLOG);
        if (listen_id_ < 0) {
            return E_FAILED;
        }
        ctx_->GetServer()->GetSocketServer()->Start(ctx_->Handle(), listen_id_);
        return E_OK;
    }

    int
    ServiceGate::CallBack_(void *ud, int type, int session, uint32_t source, DataPtr msg, size_t sz) {
        auto *g = static_cast<ServiceGate *>(ud);
        switch (type) {
            case PTYPE_TEXT:
                g->Ctrl(msg, sz);
                break;
            case PTYPE_CLIENT: {
                if (sz <= 4) {
                    g->logger->error("Invalid client message from {}", source);
                    break;
                }
                const uint8_t *id_buf = static_cast<uint8_t *>(msg.get()) + sz - 4;
                uint32_t uid = id_buf[0] | id_buf[1] << 8 | id_buf[2] << 16 | id_buf[3] << 24;
                if (auto it = g->conn_.find(uid); it != g->conn_.end()) {
                    g->ctx_->GetServer()->GetSocketServer()->Send(uid, msg, sz-4);
                    return E_OK;
                } else {
                    g->logger->error("Invalid client id {} from {}", uid, source);
                    break;
                }
            }

            case PTYPE_SOCKET:
                g->DispatchSocketMessage(std::reinterpret_pointer_cast<TinkSocketMessage>(msg), int(sz-sizeof(TinkSocketMessage)));
                break;
            default:
                break;
        }
        return E_OK;
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
            if (conn_.find(uid) != conn_.end()) {
                ctx_->GetServer()->GetSocketServer()->Close(ctx_->Handle(), uid);
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
            broker_ = ctx_->GetServer()->GetHandlerMgr()->QueryName(param_list[1]);
            return;
        }
        if (param_list[0] == "start") {
            int uid = std::stol(param_list[1]);
            if (conn_.find(uid) != conn_.end()) {
                ctx_->GetServer()->GetSocketServer()->Start(ctx_->Handle(), uid);
            }
            return ;
        }
        if (param_list[0] == "close") {
            if (listen_id_ >= 0) {
                ctx_->GetServer()->GetSocketServer()->Close(ctx_->Handle(), listen_id_);
                listen_id_ = -1;
            }
            return;
        }
        logger->error("[gate] unknown command : {}", command);
    }

    void ServiceGate::ForwardAgent_(int fd, uint32_t agent_addr, uint32_t client_addr) {
        auto it =conn_.find(fd);
        if (it != conn_.end()) {
            Connection* agent = it->second;
            agent->agent = agent_addr;
            agent->client = client_addr;
        }
    }

    void ServiceGate::DispatchSocketMessage(TinkSocketMsgPtr msg, int sz) {
        switch (msg->type) {
            case TinkSocketMessage::DATA: {
                if (auto it = conn_.find(msg->id); it != conn_.end()) {
                    Connection * c = it->second;
                    DispatchMessage(*c, msg->id, msg->buffer, msg->ud);
                } else {
                    logger->error("drop unknown connection {} message", msg->id);
                    ctx_->GetServer()->GetSocketServer()->Close(ctx_->Handle(), msg->id);
                    msg->buffer.reset();
                }
            }
            case TinkSocketMessage::CONNECT: {
                if (msg->id == listen_id_) {
                    // start listening
                    break;
                }
                if (conn_.find(msg->id) == conn_.end()) {
                    logger->error("close unknown connection {}", msg->id);
                    ctx_->GetServer()->GetSocketServer()->Close(ctx_->Handle(), msg->id);
                }
                break;
            }
            case TinkSocketMessage::CLOSE:
            case TinkSocketMessage::ERROR: {
                if (auto it = conn_.find(msg->id); it != conn_.end()) {
                    Connection *c = it->second;
                    c->buffer.Clear(*msg_pool_);
                    conn_pool_->ReusePoolItem(c);
                    conn_.erase(it);
                    Report_("%d close", msg->id);
                }
                break;
            }
            case TinkSocketMessage::ACCEPT: {
                if (conn_pool_->IsFreePoolEmpty()) {
                    ctx_->GetServer()->GetSocketServer()->Close(ctx_->Handle(), msg->ud);
                } else {
                    Connection * c = conn_pool_->GetPoolItem();
                    if (sz >= sizeof(c->remote_name)) {
                        sz = sizeof(c->remote_name) - 1;
                    }
                    c->id = msg->ud;
                    memcpy(c->remote_name, msg.get() + 1, sz);
                    c->remote_name[sz] = '\0';
                    Report_("%d open %d %s:0", c->id, c->id, c->remote_name);
                    conn_.emplace(std::make_pair(msg->ud, c));
                    logger->info("socket open:{0:x}", c->id);
                }
                break;
            }
            case TinkSocketMessage::WARNING:
                logger->error("fd (%d) send buffer (%d)K", msg->id, msg->ud);
                break;
        }
    }

    void ServiceGate::DispatchMessage(Connection &c, int id, DataPtr data, int sz) {
        c.buffer.Push(*msg_pool_, data, sz);
        for (;;) {
            int size = c.buffer.ReadHeader(*msg_pool_, header_size_);
            if (size < 0) {
                return ;
            } else if (size > 0) {
                if (size >= 0x1000000) {
                    c.buffer.Clear(*msg_pool_);
                    ctx_->GetServer()->GetSocketServer()->Close(ctx_->Handle(), id);
                    logger->error("Recv socket message > 16M");
                    return ;
                } else {
                    c.buffer.Reset();
                }
            }
        }
    }

    void ServiceGate::Forward_(Connection &c, int size) {
        int fd = c.id;
        if (fd <= 0) {
            // socket error
            return;
        }
        if (broker_) {
            DataPtr temp = std::make_shared<byte[]>(size);
            c.buffer.Read(*msg_pool_, temp.get(), size);
            ctx_->Send(0, broker_, client_tag_, fd, temp, size);
            return ;
        }
        if (c.agent) {
            DataPtr temp = std::make_shared<byte[]>(size);
            c.buffer.Read(*msg_pool_, temp.get(), size);
            ctx_->Send(c.client, c.agent, client_tag_, fd, temp, size);
        } else if (watchdog_) {
            DataPtr tmp = std::make_shared<byte[]>(size+32);
            int n = snprintf(static_cast<byte*>(tmp.get()), 32, "%d data", c.id);
            c.buffer.Read(*msg_pool_, static_cast<byte*>(tmp.get()) + n, size);
            ctx_->Send(0, watchdog_, PTYPE_TEXT, fd, tmp, size + n);
        }
    }

    void ServiceGate::Report_(const char *data, ...) {
        if (watchdog_ == 0) {
            return;
        }
        va_list ap;
        va_start(ap, data);
        DataPtr tmp(new byte[1024], std::default_delete<byte[]>());
        int n = vsnprintf(static_cast<byte*>(tmp.get()), 1024, data, ap);
        va_end(ap);
        ctx_->Send(0, watchdog_, PTYPE_TEXT, 0, tmp, n);
    }
}

