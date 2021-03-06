#include <string_view>
#include <sstream>
#include "net/harbor.h"
#include "net/server.h"
#include "base/base_module.h"
#include "error_code.h"
#include "harbor_message.h"

namespace tink::Service {
    class ServiceHarbor : public BaseModule {
    public:
        static constexpr int STATUS_WAIT = 0;
        static constexpr int STATUS_HANDSHAKE = 1;
        static constexpr int STATUS_HEADER = 2;
        static constexpr int STATUS_CONTENT = 3;
        static constexpr int STATUS_DOWN = 4;

        static constexpr int REMOTE_MAX = 256;
        static constexpr int HEADER_COOKIE_LENGTH = 12;

        typedef struct Slave_ : public noncopyable {
            int fd;
            std::shared_ptr<HarborMsgQueue> queue;
            int status;
            int length;
            int read;
            uint8_t size[4];
            BytePtr recv_buffer;
        } Slave;

        ServiceHarbor();
        Slave & GetSlave(int id);
        int Init(ContextPtr ctx, std::string_view param) override;
        void PushSocketData(TinkSocketMsgPtr message);
        int GetHarborId(int fd);
        void ReportHarborDown(int id);
        void HarborCommand(const char *msg, size_t sz, int session, uint32_t source);
        int RemoteSendName(uint32_t source, const std::string& name, int type, int session, DataPtr msg, size_t sz);
        int RemoteSendHandle(uint32_t source, uint32_t destination, int type, int session, DataPtr msg, size_t sz);
    private:

        static int MainLoop_(void *ud, int type, int session, uint32_t source, DataPtr msg, size_t sz);
        void CloseSlave_(int id);
        void DispatchQueue_(int id);
        void DispatchNameQueue_(HarborMap::iterator &node);
        void SendRemote_(int fd, const BytePtr& buffer, size_t sz, RemoteMsgHeader& cookie);
        void UpdateName_(const std::string& name, uint32_t handle);
        void Handshake_(int id);
        void ForwardLocalMessage(DataPtr msg, int sz);
        void PushQueue_(HarborMsgQueue& queue, DataPtr buffer, size_t sz, RemoteMsgHeader& header);
        ContextPtr ctx_;
        int id_;
        uint32_t slave_;
        HarborMap map_;
        std::array<Slave, REMOTE_MAX> s_;
    };



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
        ctx_->GetServer()->GetHarbor()->Start(ctx_);
        return 0;
    }

    int ServiceHarbor::MainLoop_(void *ud, int type, int session, uint32_t source, DataPtr msg, size_t sz) {
        auto* h = static_cast<ServiceHarbor*>(ud);
        switch (type) {
        case PTYPE_SOCKET: {
            auto message = std::reinterpret_pointer_cast<TinkSocketMessage>(msg);
            switch (message->type) {
                case TinkSocketMessage::DATA:
                    h->PushSocketData(message);
                    break;
                case TinkSocketMessage::ERROR:
                case TinkSocketMessage::CLOSE: {
                    int id = h->GetHarborId(message->id);
                    if (id) {
                        h->ReportHarborDown(id);
                    } else {
                        h->logger->error("unknown fd ({}) closed", message->id);
                    }
                    break;
                }
                case TinkSocketMessage::CONNECT:
                    // fd forward to this service
                    break;
                case TinkSocketMessage::WARNING: {
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
            return  E_OK;
        }
        case PTYPE_HARBOR: {
            h->HarborCommand(static_cast<const char *>(msg.get()), sz, session, source);
            return E_OK;
        }
        case PTYPE_SYSTEM: {
            RemoteMessagePtr r_msg = std::reinterpret_pointer_cast<RemoteMessage>(msg);
            if (r_msg->destination.handle == 0) {
                if (h->RemoteSendName(source, r_msg->destination.name, r_msg->type, session, r_msg->message,
                                      r_msg->size)) {
                    return E_OK;
                }
            } else {
                if (h->RemoteSendHandle(source, r_msg->destination.handle, r_msg->type, session, r_msg->message,
                                        r_msg->size)) {
                    return E_OK;
                }
            }
            return 0;
        }
        default:
            h->logger->error("recv invalid message from {:x},  type = {}", source, type);
            if (session != 0 && type != PTYPE_ERROR) {
                h->ctx_->Send(0, source, PTYPE_ERROR, session, nullptr, 0);
            }
            return  0;
        }
        return 0;
    }

    void ServiceHarbor::PushSocketData(TinkSocketMsgPtr message) {
        assert(message->type == TinkSocketMessage::DATA);
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
        auto *buffer = static_cast<uint8_t *>(message->buffer.get());
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
                    DispatchQueue_(id);
                    if (size == 0) {
                        break;
                    }

                }
                case STATUS_HEADER: {
                    // 大端4byte, 第一字节必须为0
                    int need = 4 - s->read;
                    if (size < need) {
                        memcpy( s->size + s->read, buffer, size);
                        s->read += size;
                    } else {
                        memcpy(s->size + s->read, buffer, size);
                        buffer += need;
                        size -= need;

                        if (s->size[0] != 0) {
                            logger->error("message is too long from harbor %d", id);
                            CloseSlave_(id);
                            return;
                        }
                        s->length = s->size[1] << 16 | s->size[2] << 8 | s->size[3];
                        s->read = 0;
                        s->recv_buffer = BytePtr(new byte[s->length], std::default_delete<byte[]>());
                        s->status = STATUS_CONTENT;
                        if (size == 0) {
                            return;
                        }
                    }
                }
                case STATUS_CONTENT: {
                    int need = s->length - s->read;
                    if (size < need) {
                        memcpy(s->recv_buffer.get() + s->read, buffer, size);
                        s->read += size;
                        return;
                    }
                    memcpy(s->recv_buffer.get() + s->read, buffer, need);
                    ForwardLocalMessage(s->recv_buffer, s->length);
                    s->length = 0;
                    s->read = 0;
                    s->recv_buffer = nullptr;
                    size -= need;
                    buffer += need;
                    s->status = STATUS_HEADER;
                    if (size == 0) {
                        return;
                    }
                    break;
                }
                default:
                    return;
            }
        }
    }

    void ServiceHarbor::CloseSlave_(int id) {
        Slave& s = GetSlave(id);
        s.status = STATUS_DOWN;
        if (s.fd) {
            ctx_->GetServer()->GetSocketServer()->Close(ctx_->Handle(), id);
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
        HarborMsg m;
        while (!queue->empty()) {
            SendRemote_(fd, std::reinterpret_pointer_cast<byte[]>(m.buffer), m.size, m.header);
        }
        s.queue = nullptr;
    }



    void ServiceHarbor::SendRemote_(int fd, const BytePtr& buffer, size_t sz, RemoteMsgHeader &cookie) {
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
        // 这里忽略发送错误,一旦连接断开,主循环中会收到消息
        ctx_->GetServer()->GetSocketServer()->Send(tmp);
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
            UpdateName_(rn.name, rn.handle);
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
            ctx_->GetServer()->GetSocketServer()->Start(ctx_->Handle(), fd);
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
                    // 本地id
                    for (auto& msg : *s.queue) {
                        int type = msg.header.destination >> HANDLE_REMOTE_SHIFT;
                        ctx_->Send(msg.header.source, handle, type, msg.header.session, msg.buffer, msg.size);
                    }
                    s.queue->clear();
                    s.queue.reset();
                }
            }
            return;
        }

        for (auto& msg : *queue) {
            msg.header.destination |= (handle & HANDLE_MASK);
            SendRemote_(fd, std::reinterpret_pointer_cast<byte[]>(msg.buffer), msg.size, msg.header);
            msg.buffer.reset();
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
        ctx_->GetServer()->GetSocketServer()->Send(tmp);
    }

    void ServiceHarbor::PushQueue_(HarborMsgQueue& queue, DataPtr buffer, size_t sz, RemoteMsgHeader& header) {
        HarborMsg m;
        m.header = header;
        m.buffer = std::move(buffer);
        m.size = sz;
        queue.emplace_back(m);
    }

    int ServiceHarbor::RemoteSendName(uint32_t source, const string &name, int type, int session, DataPtr msg, size_t sz) {
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
            RemoteMsgHeader header{};
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
            return RemoteSendHandle(source, val.first, type, session, msg, sz);
        }
        return 0;
    }

    int ServiceHarbor::RemoteSendHandle(uint32_t source,
                                        uint32_t destination, int type, int session, DataPtr msg, size_t sz) {
        int harbor_id = destination >> HANDLE_REMOTE_SHIFT;
        if (harbor_id == id_) {
            // 本地消息
            ctx_->Send(source, destination, type, session, msg, sz);
            return 1;
        }
        Slave& s = GetSlave(harbor_id);
        if (s.fd == 0 || s.status == STATUS_HANDSHAKE) {
            if (s.status == STATUS_DOWN) {
                // 向发送源抛出错误,告诉目标地址已经关闭
                ctx_->Send(destination, source, PTYPE_TEXT, 0, nullptr, 0);
                logger->error("drop message to harbor {} from {:x} to {:x} (session = {}, msgsz = {})",
                              harbor_id, source, destination, session, sz);
            } else {
                if (!s.queue) {
                    s.queue = std::make_shared<HarborMsgQueue>();
                }
                RemoteMsgHeader header;
                header.source = source;
                header.destination = (type << HANDLE_REMOTE_SHIFT) | (destination & HANDLE_MASK);
                header.session = session;
                PushQueue_(*s.queue, msg, sz, header);
                return 1;
            }
        } else {
            RemoteMsgHeader cookie;
            cookie.source = source;
            cookie.destination = (destination & HANDLE_MASK) | (static_cast<uint32_t>(type) << HANDLE_REMOTE_SHIFT) ;
            cookie.session = session;
            SendRemote_(s.fd, std::reinterpret_pointer_cast<byte[]>(msg), sz, cookie);
        }
        return 0;
    }


    void ServiceHarbor::ForwardLocalMessage(DataPtr msg, int sz) {
        const char * cookie = static_cast<const char *>(msg.get());
        cookie += sz - HEADER_COOKIE_LENGTH;
        RemoteMsgHeader header;
        MessageToHeader(reinterpret_cast<const uint32_t *>(cookie), header);

        uint32_t destination = header.destination;
        int type = destination >> HANDLE_REMOTE_SHIFT;
        destination = (destination & HANDLE_MASK) | (id_ << HANDLE_REMOTE_SHIFT);
        if (ctx_->Send(header.source, destination, type, header.session, msg, sz - HEADER_COOKIE_LENGTH) < 0) {
            if (type != PTYPE_ERROR) {
                // 错误类型的信息发生错误不需要回复
                ctx_->Send(destination, header.source, PTYPE_ERROR, header.session, nullptr, 0);
            }
            logger->error("unknown destination :{:x} from :{:x} type({})", destination, header.source, type);
        }
    }

}

extern "C" {
tink::BaseModule* CreateModule() {
    return new tink::Service::ServiceHarbor();
}
};