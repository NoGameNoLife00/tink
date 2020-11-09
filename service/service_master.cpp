#include "service_master.h"
#include <memory>
#include <base_module.h>
#include <string_util.h>
#include <error_code.h>
#include <socket_server.h>


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
        BytePtr start(new byte[]{"start"}, std::default_delete<byte[]>());
        ctx_->Send(0, gate, PTYPE_TEXT, 0, tmp, n);
        ctx_->Send(0, gate, PTYPE_TEXT, 0, start, 5);

        ctx_->SetCallBack(MainLoop_, this);
        return E_OK;
    }

    void ServiceMaster::Release() {
        BaseModule::Release();
    }

    void ServiceMaster::Signal(int signal) {
        BaseModule::Signal(signal);
    }

    int ServiceMaster::MainLoop_(Context &ctx, void *ud, int type, int session, uint32_t source, DataPtr msg, size_t sz) {
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
        auto *name = static_cast<const byte *>msg.get();
        name += 4;

        if (handle == 0) {
            RequestName(name, sz);
        } else if (handle < REMOTE_MAX) {

        } else {

        }

        return 0;
    }

    void ServiceMaster::DispatchSocket(TinkSocketMsgPtr msg, int sz) {
        int id = SocketId(msg->id);
        switch (msg->type) {
            case TINK_SOCKET_TYPE_CONNECT:
                assert(id);
                OnConnected_(id);
                break;
            case TINK_SOCKET_TYPE_ERROR:
                logger->error("socket error on harbor {}", id);
                // ����, �ر�����
            case TINK_SOCKET_TYPE_CLOSE:
                CloseHarbor_(id);
                break;
            default:
                logger->error("invalid socket message type {}", msg->type);
                break;
        }
    }

    int ServiceMaster::SocketId(int id) {
        auto it = std::find(remote_fd.begin(), remote_fd.end(), id);
        return std::distance(remote_fd.begin(), it);
    }

    void ServiceMaster::OnConnected_(int id) {
        Broadcast(*remote_addr[id], remote_addr[id]->length(), id);
        connected[id] = true;
        for (int i = 1; i < REMOTE_MAX; i++) {
            if (i == id) {
                continue;
            }
            StringPtr addr = remote_addr[i];
            if (!addr || connected[i] == false) {
                continue;
            }
            SendTo_(id, reinterpret_cast<const void *>(*addr->data()), addr->length(), i);
        }
    }

    void ServiceMaster::Broadcast(std::string_view name, size_t sz, uint32_t handle) {
        for (int i = 1; i < REMOTE_MAX; i++) {
            int fd = remote_fd[i];
            if (fd < 0 || connected[i] == false) {
                continue;
                SendTo_(i, name.data(), name.length(), handle);
            }
        }
    }

    static inline void ToBigEndian(uint8_t *buffer, uint32_t n) {
        buffer[0] = (n >> 24) & 0xff;
        buffer[1] = (n >> 16) & 0xff;
        buffer[2] = (n >> 8) & 0xff;
        buffer[3] = n & 0xff;
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
        if (SOCKET_SERVER.Send(remote_fd[id], data, sz)) {
            logger->error("Harbor {} : send error", id);
        }
    }

    void ServiceMaster::CloseHarbor_(int harbor_id) {
        if (connected[harbor_id]) {
            SOCKET_SERVER.Close(ctx_->Handle(), remote_fd[harbor_id]);
            remote_fd[harbor_id] = -1;
            remote_addr[harbor_id].reset();
        }
    }

    void ServiceMaster::RequestName_(const byte *buffer, size_t sz) {
        std::string name(buffer, sz);
        auto it = map.find(name);
        if (it == map.end()) {
            return;
        }
        Broadcast(name, name.length(), it->second);
    }

    void ServiceMaster::UpdateName_(uint32_t handle, const byte *buffer, int sz) {
        std::string name(buffer, sz);
        auto it = map.find(name);
        if (it == map.end()) {
            bool ret;
            std::tie(it, ret) = map.emplace(std::move(name), 0);
        }
        it->second = handle;
        Broadcast(name, name.length(), handle);
    }

}

