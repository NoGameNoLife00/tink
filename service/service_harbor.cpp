#include <string_view>
#include <error_code.h>
#include <sstream>
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

    int ServiceHarbor::MainLoop_(Context &ctx, void *ud, int type, int session, uint32_t source, DataPtr &msg, size_t sz) {
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



    void ServiceHarbor::SendRemote_(int fd, BytePtr buffer, size_t sz, ServiceHarbor::RemoteMsgHeader &cookie) {
        size_t sz_header = sz + sizeof(cookie);
        if (sz_header > UINT32_MAX) {
            logger->error("remote message from {0:08x} to :{0:08x} is too large.", cookie.source, cookie.destination);
            return;
        }
        std::shared_ptr<uint8_t[]> send_buf(new uint8_t[sz_header+4], std::default_delete<uint8_t[]>());

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
        ctx_->Send(0, slave_, PTYPE_TEXT, 0, std::dynamic_pointer_cast<void>(down), n);
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

        }
        }
    }

}
