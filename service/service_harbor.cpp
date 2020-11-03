#include <string_view>
#include <error_code.h>
#include <sstream>
#include <utility>
#include <harbor.h>
#include <socket_server.h>
#include "service_harbor.h"

namespace tink::Service {


    static inline void ToBigEndian(uint8_t *buffer, uint32_t n) {
        buffer[0] = (n >> 24) & 0xff;
        buffer[1] = (n >> 16) & 0xff;
        buffer[2] = (n >> 8) & 0xff;
        buffer[3] = n & 0xff;
    }

    static inline uint32_t FromBigEndian(uint32_t n) {
        union {
            uint32_t big;
            uint8_t bytes[4];
        } u;
        u.big = n;
        return u.bytes[0] << 24 | u.bytes[1] << 16 | u.bytes[2] << 8 | u.bytes[3];
    }

    static inline void HeaderToMessage(const ServiceHarbor::RemoteMsgHeader &header, uint8_t *message) {
        ToBigEndian(message, header.source);
        ToBigEndian(message + 4, header.destination);
        ToBigEndian(message + 8, header.session);
    }

    static inline void MessageToHeader(const uint32_t *message, ServiceHarbor::RemoteMsgHeader &header) {
        header.source = FromBigEndian(message[0]);
        header.destination = FromBigEndian(message[1]);
        header.session = FromBigEndian(message[2]);
    }


    ServiceHarbor::ServiceHarbor() {

    }

    int ServiceHarbor::Init(ContextPtr ctx, std::string_view param) {
        if (param.empty()) {
            return E_FAILED;
        }
        ctx_ = ctx;

        InitLog();
        std::istringstream is(param.data());
        is >> id_ >> slave_;
        if (slave_ == 0) {
            return E_FAILED;
        }
        ctx_->SetCallBack(MainLoop_, this);
        HARBOR.Start(ctx_);
        return 0;
    }

    int ServiceHarbor::MainLoop_(Context &ctx, void *ud, int type, int session, uint32_t source, DataPtr msg, size_t sz) {
        auto* h = static_cast<ServiceHarbor*>(ud);
        switch (type) {
        case PTYPE_SOCKET: {
            auto message = std::dynamic_pointer_cast<TinkSocketMessage>(msg);
            switch (message->type) {
                case TINK_SOCKET_TYPE_DATA:
                    h->PushSocketData(message);
                    break;
                case TINK_SOCKET_TYPE_ERROR:
                case TINK_SOCKET_TYPE_CLOSE: {
                    int id = h->GetHarborId(message->id);
                    if (id) {
                        h->ReportHarborDown(id);
                    } else {
                        h->logger->error("unknown fd ({}) closed", message->id);
                    }
                    break;
                }
                case TINK_SOCKET_TYPE_CONNECT:
                    // fd forward to this service
                    break;
                case TINK_SOCKET_TYPE_WARNING: {
                    int id = h->GetHarborId(message->id);
                    if (id) {
                        h->logger->error("message haven't send to harbor ({}) reach {} K", id, message->ud);
                    }
                    break;
                }
                default:
                    h->logger->error("recv invalid socket message type {}", type);
                    break;
                }
            return  0;
        }
        case PTYPE_HARBOR: {
            h->HarborCommand(static_cast<const char *>(msg.get()), sz, session, source);
            return 0;
        }
        case PTYPE_SYSTEM: {
            RemoteMessagePtr r_msg = std::dynamic_pointer_cast<RemoteMessage>(msg);
            if (r_msg->destination.handle == 0) {
                if (RemoteSendName_())
            }
        }
        }
        return 0;
    }

    void ServiceHarbor::PushSocketData(TinkSocketMsgPtr message) {
        assert(message->type == TINK_SOCKET_TYPE_DATA);
        int fd = message->id;
        int id;
        Slave *s = nullptr;
        for (int i = 1; i < s_.max_size(); i++) {
            if (s_[i].fd == fd) {
                s = &s_[i];
                id = i;
            }
        }
        if (!s) {
            logger->error("invalid socket fd ({}) data", fd);
            return;
        }

//        BytePtr buffer = std::dynamic_pointer_cast<byte[]>(message->buffer) ;
        uint8_t *buffer = static_cast<uint8_t *>(message->buffer.get());
        int size = message->ud;
        for (;;) {
            switch (s->status) {
                case STATUS_HANDSHAKE: {
                    // check id
                    uint8_t remote_id = buffer[0];
                    if (remote_id != id) {
                        logger->error("invalid shakehand id ({}) from fd = {} , harbor = {}", id, fd, remote_id);
                        CloseSlave_(id);
                        return;
                    }
                    ++buffer;
                    --size;
                    s->status = STATUS_HEADER;

                }

            }
        }
    }

    void ServiceHarbor::CloseSlave_(int id) {
        Slave& s = GetSlave(id);
        s.status = STATUS_DOWN;
        if (s.fd) {
            SOCKET_SERVER.Close(ctx_->Handle(), id);
        }
        if (s.queue) {
            s.queue->clear();
            s.queue.reset();
        }
    }

    ServiceHarbor::Slave &ServiceHarbor::GetSlave(int id) {
        return s_[id];
    }

    void ServiceHarbor::DispatchQueue_(int id) {
        Slave& s = GetSlave(id);
        int fd = s.fd;
        assert(fd != 0);
        auto queue = s.queue;
        if (!queue) {
            return;
        }
        HarborMsgPtr m;
        while ((m = queue->front()) != nullptr) {

        }
    }



    void ServiceHarbor::SendRemote_(int fd, const BytePtr& buffer, size_t sz, ServiceHarbor::RemoteMsgHeader &cookie) {
        size_t sz_header = sz + sizeof(cookie);
        if (sz_header > UINT32_MAX) {
            logger->error("remote message from {0:08x} to :{0:08x} is too large.", cookie.source, cookie.destination);
            return;
        }
        auto *send_buf = new uint8_t[sz_header + 4];
        ToBigEndian(send_buf, sz_header);
        memcpy(send_buf + 4, buffer.get(), sz);
        HeaderToMessage(cookie, send_buf);

        SocketSendBuffer tmp;
        tmp.id = fd;
        tmp.type = SOCKET_BUFFER_RAWPOINTER;
        tmp.buffer = std::shared_ptr<uint8_t>(send_buf, std::default_delete<uint8_t[]>());
        tmp.sz = sz_header + 4;
        // ������Է��ʹ���,һ�����ӶϿ�,��ѭ���л��յ���Ϣ
        SOCKET_SERVER.Send(tmp);
    }

    int ServiceHarbor::GetHarborId(int fd) {
        for (int i = 1; i < REMOTE_MAX; i++) {
            if (s_[i].fd == fd) {
                return i;
            }
        }
        return 0;
    }

    void ServiceHarbor::ReportHarborDown(int id) {
        BytePtr down(new byte[64], std::default_delete<byte[]>());
        int n = sprintf(down.get(), "D %d", id);
        ctx_->Send(0, slave_, PTYPE_TEXT, 0, down, n);
    }

    void ServiceHarbor::HarborCommand(const char *msg, size_t sz, int session, uint32_t source) {
        const char *name = msg + 2;
        int s = sz - 2;
        switch (msg[0]) {
        case 'N': {
            if (s <= 0 || s >= GLOBALNAME_LENGTH) {
                logger->error("invalid global name {}", name);
                return;
            }
            RemoteName rn {};
            memcpy(rn.name, name, s);
            rn.handle = source;
            break;
        }
        case 'S':
        case 'A': {
            int fd = 0, id = 0;
            string buffer(name, s);
            std::istringstream ss(buffer);
            ss >> fd >> id;
            if (fd == 0 || id <= 0 || id >= REMOTE_MAX) {
                logger->error("invalid command {} {}", msg[0], buffer);
                return;
            }
            Slave& slave = GetSlave(id);
            if (slave.fd != 0) {
                logger->error("harbor {} already exist", id);
                return;
            }
            slave.fd = fd;
            SOCKET_SERVER.Start(ctx_->Handle(), fd);
            Handshake_(id);
            if (msg[0] == 'S') {
                slave.status = STATUS_HANDSHAKE;
            } else {
                slave.status = STATUS_HEADER;
                DispatchQueue_(id);
            }
            break;
        }
        default:
            logger->error("unknown command %s, msg");
            return;
        }
    }

    void ServiceHarbor::UpdateName_(const std::string& name, uint32_t handle) {
        auto it = map_.find(name);
        if (it == map_.end()) {
            bool ret;
            std::tie(it, ret) = map_.emplace(name, std::pair(handle, nullptr));
        }
        std::pair<int, HarborMsgQueuePtr>& val = it->second;
        val.first = handle;
        if (val.second) {
            DispatchNameQueue_(it);
            val.second.reset();
        }
    }

    void ServiceHarbor::DispatchNameQueue_(HarborMap::iterator &node) {
        HarborValue& val = node->second;
        HarborMsgQueuePtr queue = val.second;
        uint32_t handle = val.first;
        auto & name = node->first;
        int harbor_id = handle >> HANDLE_REMOTE_SHIFT;
        Slave& s = GetSlave(harbor_id);
        int fd = s.fd;
        if (fd == 0) {
            if (s.status == STATUS_DOWN) {
                logger->error("drop message to {} (in harbor {})", name, harbor_id);
            } else {
                if (!s.queue) {
                    s.queue = queue;
                    val.second.reset();
                } else {
                    HarborMsgPtr msg;
                    s.queue->splice(s.queue->end(), *queue);
                }
                if (harbor_id == (slave_ >> HANDLE_REMOTE_SHIFT)) {
                    // ����id
                    for (auto& msg : *s.queue) {
                        int type = msg->header.destination >> HANDLE_REMOTE_SHIFT;
                        ctx_->Send(msg->header.source, handle, type, msg->header.session, msg->buffer, msg->size);
                    }
                    s.queue->clear();
                    s.queue.reset();
                }
            }
            return;
        }

        for (auto& msg : *queue) {
            msg->header.destination |= (handle & HANDLE_MASK);
            SendRemote_(fd, std::dynamic_pointer_cast<byte[]>(msg->buffer), msg->size, msg->header);
            msg->buffer.reset();
        }
        queue->clear();
    }

    void ServiceHarbor::Handshake_(int id) {
        Slave& s = GetSlave(id);
        uint8_t* handshake = new uint8_t[1] {static_cast<uint8_t>(id_)};
        SocketSendBuffer tmp;
        tmp.id = s.fd;
        tmp.type = SOCKET_BUFFER_RAWPOINTER;
        tmp.buffer = DataPtr(handshake, std::default_delete<uint8_t[]>());
        tmp.sz = 1;
        SOCKET_SERVER.Send(tmp);
    }

    void ServiceHarbor::PushQueue_(HarborMsgQueue& queue, DataPtr buffer, size_t sz, RemoteMsgHeader& header) {
        HarborMsg m;
        m.header = header;
        m.buffer = std::move(buffer);
        m.size = sz;
        queue.emplace_back(m);
    }

    int ServiceHarbor::RemoteSendName_(uint32_t source, const string &name, int type, int session, DataPtr msg, size_t sz) {
        auto it = map_.find(name);
        if (it == map_.end()) {
            bool ret;
            std::tie(it, ret) = map_.emplace(name, std::pair(0, nullptr));
        }
        HarborValue& val = it->second;
        if (val.first == 0) {
            if (!val.second) {
                val.second.reset(new HarborMsgQueue);
            }
            RemoteMsgHeader header;
            header.source = source;
            header.destination = type << HANDLE_REMOTE_SHIFT;
            header.session = session;
            PushQueue_(*val.second, msg, sz, header);
            std::string query = "Q " + name;
            int len = query.length()+1;
            BytePtr tmp(new byte[len], std::default_delete<byte[]>());
            query.copy(tmp.get(), query.length(), 0);
            ctx_->Send(0, slave_, PTYPE_TEXT, 0, tmp, len);
            return 1;
        } else {
            return RemoteSendHandle()
        }
        return 0;
    }

}