#include <memory>
#include "base/base_module.h"
#include "base/string_util.h"
#include "net/socket_server.h"
#include "error_code.h"
#include "service_master.h"
#include "harbor_message.h"
extern "C" {
tink::BaseModule* CreateModule() {
    return new tink::Service::ServiceMaster();
}
};

namespace tink::Service {
    int ServiceMaster::Init(ContextPtr ctx, std::string_view param) {
        BytePtr tmp(new byte[param.length()+32], std::default_delete<byte[]>());
        ctx_ = ctx;
        sprintf(tmp.get(), "gate L ! %s %d %d 0", param.data(), PTYPE_HARBOR, REMOTE_MAX);
        std::string&& gate_addr = ctx_->Command("LAUNCH", tmp.get());
        if (gate_addr.empty()) {
            logger->error("master: launch gate failed");
            return E_FAILED;
        }
        uint32_t gate = strtoul(gate_addr.data()+1, nullptr, 16);
        if (gate == 0) {
            logger->error("master : launch gate invalid {}", gate_addr);
            return E_FAILED;
        }
        std::string&& self_addr = ctx_->Command("REG", "");
        int n = sprintf(tmp.get(),"broker %s", self_addr.data());
        byte* s = new byte[6]{"start"};
        BytePtr start(s, std::default_delete<byte[]>());
        ctx_->Send(0, gate, PTYPE_TEXT, 0, tmp, n);
        ctx_->Send(0, gate, PTYPE_TEXT, 0, start, 5);

        ctx_->SetCallBack(this, 0, 0, 0, tink::DataPtr(), 0);
        return E_OK;
    }

    void ServiceMaster::Release() {
        for (int i = 0; i < REMOTE_MAX; i++) {
            int fd = remote_fd_[i];
            if (fd >= 0) {
                assert(ctx_);
                SOCKET_SERVER.Close(ctx_->Handle(), fd);
            }
            remote_addr_[i].reset();
        }
        name_map_.clear();
    }

    void ServiceMaster::Signal(int signal) {
        BaseModule::Signal(signal);
    }

    int ServiceMaster::MainLoop_(void *ud, int type, int session, uint32_t source, DataPtr msg, size_t sz) {
        auto m = reinterpret_cast<ServiceMaster*>(ud);
        if (type == PTYPE_TEXT) {
            m->DispatchSocket(std::reinterpret_pointer_cast<TinkSocketMessage>(msg), sz);
            return E_OK;
        }
        if (type != PTYPE_HARBOR) {
            m->logger->error("None harbor message recv from {:x} (type = {})", source, type);
            return E_OK;
        }
        assert(sz >= 4);
        auto *handlen = static_cast<const uint8_t *>(msg.get());
        uint32_t handle = handlen[0]<<24 | handlen[1]<<16 | handlen[2]<<8 | handlen[3];
        sz -= 4;
        auto *name = static_cast<const byte *>(msg.get());
        name += 4;
        string name_str(name, sz);
        if (handle == 0) {
            m->RequestName_(name);
        } else if (handle < REMOTE_MAX) {
            m->UpdateAddress_(handle, name);
        } else {
            m->UpdateName_(handle, name);
        }

        return 0;
    }

    void ServiceMaster::DispatchSocket(TinkSocketMsgPtr msg, int sz) {
        int id = SocketId(msg->id);
        switch (msg->type) {
            case TinkSocketMessage::CONNECT:
                assert(id);
                OnConnected_(id);
                break;
            case TinkSocketMessage::ERROR:
                logger->error("socket error on harbor {}", id);
                // 继续, 关闭连接
            case TinkSocketMessage::CLOSE:
                CloseHarbor_(id);
                break;
            default:
                logger->error("invalid socket message type {}", msg->type);
                break;
        }
    }

    int ServiceMaster::SocketId(int id) {
        auto it = std::find(remote_fd_.begin() + 1, remote_fd_.end(), id);
        return std::distance(remote_fd_.begin(), it);
    }

    void ServiceMaster::OnConnected_(int id) {
        Broadcast(remote_addr_[id]->data(), remote_addr_[id]->length(), id);
        connected_[id] = true;
        for (int i = 1; i < REMOTE_MAX; i++) {
            if (i == id) {
                continue;
            }
            StringPtr addr = remote_addr_[i];
            if (!addr || !connected_[i]) {
                continue;
            }
            SendTo_(id, reinterpret_cast<const void *>(*addr->data()), addr->length(), i);
        }
    }

    void ServiceMaster::Broadcast(const byte *name, size_t sz, uint32_t handle) {
        for (int i = 1; i < REMOTE_MAX; i++) {
            int fd = remote_fd_[i];
            if (fd < 0 || !connected_[i]) {
                continue;
            }
            SendTo_(i, name, sz, handle);
        }
    }

    void ServiceMaster::SendTo_(int id, const void *buf, int sz, uint32_t handle) {
        auto *buffer = new uint8_t[4 + sz +12];
        ToBigEndian(buffer, sz+12);
        memcpy(buffer + 4, buf, sz);
        ToBigEndian(buffer + 4 + sz, 0);
        ToBigEndian(buffer + 4 + sz + 4, handle);
        ToBigEndian(buffer + 4 + sz +8, 0);
        sz += 4 + 12;
        DataPtr data(buffer, std::default_delete<uint8_t[]>());
        if (SOCKET_SERVER.Send(remote_fd_[id], data, sz)) {
            logger->error("Harbor {} : send error", id);
        }
    }

    void ServiceMaster::CloseHarbor_(int harbor_id) {
        if (connected_[harbor_id]) {
            SOCKET_SERVER.Close(ctx_->Handle(), remote_fd_[harbor_id]);
            remote_fd_[harbor_id] = -1;
            remote_addr_[harbor_id].reset();
        }
    }

    void ServiceMaster::RequestName_(const std::string& name) {
        auto it = name_map_.find(name);
        if (it == name_map_.end()) {
            return;
        }
        Broadcast(name.data(), name.length(), it->second);
    }

    void ServiceMaster::UpdateName_(uint32_t handle, const std::string &name) {
        auto it = name_map_.find(name);
        if (it == name_map_.end()) {
            bool ret;
            std::tie(it, ret) = name_map_.emplace(name, 0);
        }
        it->second = handle;
        Broadcast(name.data(), name.length(), handle);
    }

    ServiceMaster::ServiceMaster() {
        for (int i = 0; i < REMOTE_MAX; i++) {
            remote_fd_[i] = -1;
        }
    }

    void ServiceMaster::UpdateAddress_(int harbor_id, const std::string &addr) {
        if (remote_fd_[harbor_id] >= 0) {
            CloseHarbor_(harbor_id);
        }
        remote_addr_[harbor_id] = std::make_shared<std::string>(addr);
        ConnectTo_(harbor_id);
    }

    void ServiceMaster::ConnectTo_(int id) {
        assert(!connected_[id]);
        StringPtr ip_address = remote_addr_[id];
        StringList out;
        StringUtil::Split(*ip_address, ':', out);
        if (out.size() < 2) {
            logger->error("Harbor {} : address invalid ({})", id, ip_address);
            return;
        }
        std::string& ip = out[0];
        int port = std::stol(out[1]);
        logger->info("Master connect to harbor({}) {}:{}", id, ip, port);
        remote_fd_[id] = SOCKET_SERVER.Connect(ctx_->Handle(), ip, port);
    }

}

