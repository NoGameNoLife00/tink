#include <socket_server.h>
#include <unistd.h>
#include <cassert>
#include <cstring>
#include <error_code.h>
#include <handle_storage.h>
#include <netdb.h>
#include <scope_guard.h>

// EAGAIN and EWOULDBLOCK may be not the same value.
#if (EAGAIN != EWOULDBLOCK)
#define AGAIN_WOULDBLOCK EAGAIN : case EWOULDBLOCK
#else
#define AGAIN_WOULDBLOCK EAGAIN
#endif


#define PRIORITY_HIGH 0
#define PRIORITY_LOW 1

#define WARNING_SIZE (1024*1024)

namespace tink {
    static const int MAX_INFO = 128;

    struct SocketMessage {
        int id; // socket池中的id
        uintptr_t opaque; // 服务地址
        int ud; // 字节数
        DataPtr data; // 数据
    };

    struct SendObject {
        DataPtr buffer;
        size_t sz;

        void InitFromSendBuffer(SocketSendBuffer &buf) {
            buffer = buf.buffer;
            sz = buf.sz;
        }
        void Init(char * object, size_t size) {
            buffer = std::shared_ptr<byte>(object);
            this->sz = size;
        }
    };


    struct RequestStart {
        int id;
        uintptr_t opaque;
    };
    struct RequestBind {
        int id;
        int fd;
        uintptr_t opaque;
    };

    struct RequestListen {
        int id;
        int fd;
        uintptr_t opaque;
        char host[1];
    };
    struct RequestClose {
        int id;
        int shutdown;
        uintptr_t opaque;
    };
    struct RequestOpen {
        int id;
        int port;
        uintptr_t opaque;
        char host[1];
    };
    struct RequestSend {
        int id;
        size_t sz;
        char *buffer;
    };
    struct RequestSendUdp {
        RequestSend send;
        uint8_t address[UDP_ADDRESS_SIZE];
    };
    struct RequestSetUdp {
        int id;
        uint8_t address[UDP_ADDRESS_SIZE];
    };
    struct RequestSetOpt {
        int id;
        int what;
        int value;
    };
    struct RequestUdp {
        int id;
        int fd;
        int family;
        uintptr_t opaque;
    };

    struct RequestPackage {
        RequestPackage() = default;
        uint8_t header[8]{};	// 6 bytes dummy
        union {
            char buffer[256];
            RequestOpen open;
            RequestSend send;
            RequestSendUdp send_udp;
            RequestClose close;
            RequestListen listen;
            RequestBind bind;
            RequestStart start;
            RequestSetOpt setopt;
            RequestUdp udp;
            RequestSetUdp set_udp;
        } u{};
        uint8_t dummy[256]{};
    };


    enum class SocketType {
        DATA = 1,
        CONNECT = 2,
        CLOSE = 3,
        ACCEPT = 4,
        ERROR = 5,
        UDP = 6,
        WARNING = 7,
    };



    void SocketServer::UpdateTime(uint64_t time) {
        time_ = time;
    }

    BytePtr StringMessage(std::string_view str) {
        BytePtr ptr(new byte[str.length()+1], std::default_delete<byte[]>());
        memcpy(ptr.get(), str.data(), str.length());
        ptr[str.length()] = 0;
        return ptr;
    }

    int SocketServer::Poll_(SocketMessage &result, int &more) {
        for (;;) {
            if (check_ctrl_) {
                if (HasCmd_()) {
                    int type = CtrlCmd_(result);
                    if (type != -1) {
                        ClearClosedEvent_(result, type);
                        return type;
                    } else
                        continue;
                } else {
                    check_ctrl_ = 0;
                }
            }
            if (event_index_ == event_n_) {
                event_n_ = poll_->Wait(ev_);
                check_ctrl_ = 1;
                more = 0;
                event_index_ = 0;
                if (event_n_ <= 0) {
                    event_n_ = 0;
                    if (errno == EINTR) {
                        continue;
                    }
                    return SOCKET_NONE;
                }
            }
            Event &e = ev_[event_index_++];
            Socket *s = static_cast<Socket *>(e.s);
            if (s == nullptr) {
                continue;
            }
            switch (s->GetType()) {
                case Socket::Type::CONNECTING:
                    return ReportConnect_(*s, result);
                case Socket::Type::LISTEN: {
                    int ok = ReportAccept_(*s, result);
                    if (ok > 0) {
                        return SOCKET_ACCEPT;
                    } if (ok < 0 ) {
                        return SOCKET_ERR;
                    }
                    // when ok == 0, retry
                    break;
                }
                case Socket::Type::INVALID:
                    fprintf(stderr, "socket server: invalid socket\n");
                break;
                default:
                    if (e.read) {
                        int type;
                        if (s->GetProtocol() == SocketProtocol::TCP) {
                            type = ForwardMessageTcp_(*s, result);
                        } else {
                            type = ForwardMessageUpd_(*s, result);
                            if (type == SOCKET_UDP) {
                                // try read again
                                --event_index_;
                                return SOCKET_UDP;
                            }
                        }
                        if (e.write && type != SOCKET_CLOSE && type != SOCKET_ERR) {
                            // Try to dispatch write message next step if write flag set.
                            e.read = false;
                            --event_index_;
                        }
                        if (type == -1)
                            break;
                        return type;
                    }
                if (e.write) {
                    int type = SendBuffer_(*s, result);
                    if (type == -1)
                        break;
                    return type;
                }
                if (e.error) {
                    // close when error
                    int code = SocketApi::GetSocketError(s->GetSockFd());
                    const char * err = nullptr;
                    err = strerror(code);
                    ForceClose_(*s, result);
                    result.data = StringMessage(err);
                    return SOCKET_ERR;
                }
                if(e.eof) {
                    ForceClose_(*s, result);
                    return SOCKET_CLOSE;
                }
                break;
            }
        }
        return 0;
    }

    int SocketServer::HasCmd_() {
        struct timeval tv = {0,0};
        int retval;
        FD_SET(recv_ctrl_fd, &rfds_);

        retval = select(recv_ctrl_fd + 1, &rfds_, nullptr, nullptr, &tv);
        if (retval == 1) {
            return 1;
        }

        return 0;
    }


    static void BlockReadPipe(int pipefd, void *buffer, int sz) {
        for (;;) {
            int n = read(pipefd, buffer, sz);
            if (n<0) {
                if (errno == EINTR)
                    continue;
                fprintf(stderr, "socket server : read pipe error %s.\n", strerror(errno));
                return;
            }
            // must atomic read from a pipe
            assert(n == sz);
            return;
        }
    }

    int SocketServer::CtrlCmd_(SocketMessage &result) {
        int fd = recv_ctrl_fd;
        // the length of message is one byte, so 256+8 buffer size is enough.
        uint8_t buffer[256];
        uint8_t header[2];
        BlockReadPipe(fd, header, sizeof(header));
        int type = header[0];
        int len = header[1];
        BlockReadPipe(fd, buffer, len);
        // ctrl command only exist in local fd, so don't worry about endian.
        switch (type) {
            case 'S':
                return StartSocket_((RequestStart*)(buffer), result);
            case 'B':
                return BindSocket_((RequestBind *)buffer, result);
            case 'L':
                return ListenSocket_((RequestListen *)buffer, result);
            case 'K':
                return CloseSocket_((RequestClose *)buffer, result);
            case 'O':
                return OpenSocket((RequestOpen *)buffer, result);
            case 'X':
                result.opaque = 0;
                result.id = 0;
                result.ud = 0;
                result.data = nullptr;
                return SOCKET_EXIT;
            case 'D':
            case 'P': {
                int priority = (type == 'D') ? PRIORITY_HIGH : PRIORITY_LOW;
                auto * request = (RequestSend*) buffer;
                int ret = SendSocket_(request, result, priority, nullptr);
                DecSendingRef_(request->id);
                return ret;
            }
            case 'A': {
                auto * rsu = (RequestSendUdp *)buffer;
                return SendSocket_(&rsu->send, result, PRIORITY_HIGH, rsu->address);
            }
            case 'C':
                return SetUdpAddress_((RequestSetUdp *) buffer, result);
            case 'T':
                SetOptSocket_((RequestSetOpt *)buffer);
                return SOCKET_NONE;
            case 'U':
                AddUdpSocket_((RequestUdp *) buffer);
                return SOCKET_NONE;
            default:
                fprintf(stderr, "socket server: Unknown ctrl %c.\n",type);
                return SOCKET_NONE;
        };

        return SOCKET_NONE;
    }

    int SocketServer::Init(uint64_t time) {
        int fd[2];
        PollerPtr poll(Poller::NewDefaultPoller());
        if (poll->Invalid()) {
            fprintf(stderr, "socket server: create event pool failed.\n");
            return SOCKET_NONE;
        }
        if (pipe(fd)) {
            fprintf(stderr, "socket server: create socket pair failed.\n");
            return SOCKET_NONE;
        }
        if (poll->Add(fd[0], nullptr)) {
            fprintf(stderr, "socket server: can't add server fd to event pool.\n");
            close(fd[0]);
            close(fd[1]);
            return SOCKET_NONE;
        }

        time_ = time;
        poll_ = poll;
        recv_ctrl_fd = fd[0];
        send_ctrl_fd = fd[1];
        check_ctrl_ = 1;

        alloc_id_ = 0;
        event_n_ = 0;
        event_index_ = 0;

        for (auto& s : slot_) {
            s = std::make_shared<Socket>();
        }

        FD_ZERO(&rfds_);
        assert(recv_ctrl_fd < FD_SETSIZE);
        return 0;
    }


    SocketPtr SocketServer::NewSocket_(int id, int fd, SocketProtocol protocol, uintptr_t opaque, bool add) {
        SocketPtr s = GetSocket(id);
        assert(s->GetType() == Socket::Type::RESERVE);

        if (add) {
            if (poll_->Add(fd, &s)) {
                s->SetType(Socket::Type::INVALID);
                return nullptr;
            }
        }
        s->Init(id, fd, protocol, opaque);
        return s;
    }

    void SocketServer::Destroy() {
        SocketMessage dummy;
        for (auto s : slot_) {
            if (s->GetType() != Socket::Type::RESERVE) {
                s->Destroy();
            }
        }
        close(send_ctrl_fd);
        close(recv_ctrl_fd);
        printf("error pool reset!!!!!!!!!!!!!!!!!!!!");
        poll_.reset();
    }

    void SocketServer::FreeWbList(WriteBufferList &list) {

    }

    static WriteBufferPtr AppendSendBuffer_(WriteBufferList &s, RequestSend *request) {
        WriteBufferPtr buf = std::make_shared<WriteBuffer>();
        buf->ptr = static_cast<char*>(request->buffer);
        buf->sz = request->sz;
        buf->buffer.reset(request->buffer);
        s.emplace_back(buf);
        return buf;
    }

    static void AppendSendBuffer(SocketPtr s, RequestSend *request) {
        WriteBufferPtr buf = AppendSendBuffer_(s->GetHigh(), request);
        s->AddWbSize(buf->sz);
    }

    static void AppendSendBufferLow(SocketPtr s, RequestSend *request) {
        WriteBufferPtr buf = AppendSendBuffer_(s->GetLow(), request);
        s->AddWbSize(buf->sz);
    }

    static void AppendSendBufferUdp(SocketPtr s, int priority, RequestSend *request, const uint8_t udp_address[UDP_ADDRESS_SIZE]) {
        auto&& wl = (priority == PRIORITY_HIGH) ? &s->GetHigh() : &s->GetLow();
        WriteBufferPtr buf = AppendSendBuffer_(*wl, request);
        memcpy(buf->upd_address, udp_address, UDP_ADDRESS_SIZE);
        s->AddWbSize(buf->sz);
    }


    void SocketServer::ForceClose_(Socket &s, SocketMessage &result) {
        result.id = s.GetId();
        result.ud = 0;
        result.data = nullptr;
        result.opaque = s.GetOpaque();
        Socket::Type type = s.GetType();
        if (type == Socket::Type::INVALID) {
            return;
        }
        assert(type != Socket::Type::RESERVE);
        FreeWbList(s.GetHigh());
        FreeWbList(s.GetLow());
        if (type != Socket::Type::PACCEPT && type != Socket::Type::PLISTEN) {
            poll_->Del(s.GetSockFd());
        }
        s.mutex.lock();
        if (type != Socket::Type::BIND) {
            if (close(s.GetSockFd()) < 0) {
                perror("close socket:");
            }
        }
        s.SetType(Socket::Type::INVALID);
        s.GetDWBuffer().reset();
        s.mutex.unlock();
    }

    void SocketServer::Exit() {
        RequestPackage request;
        SendRequest_(request, 'x', 0);
    }

    void SocketServer::SendRequest_(RequestPackage &request, char type, int len) const {
        request.header[6] = (uint8_t)type;
        request.header[7] = (uint8_t)len;
        for (;;) {
            ssize_t n = write(send_ctrl_fd, &request.header[6], len + 2);
            if (n<0) {
                if (errno != EINTR) {
                    fprintf(stderr, "socket server : send ctrl command error %s.\n", strerror(errno));
                }
                continue;
            }
            assert(n == len+2);
            return;
        }
    }

    void SocketServer::Close(uintptr_t opaque, int id) {
        RequestPackage request;
        request.u.close.id = id;
        request.u.close.shutdown = 0;
        request.u.close.opaque = opaque;
        SendRequest_(request, 'K', sizeof(request.u.close));
    }

    void SocketServer::Shutdown(uintptr_t opaque, int id) {
        RequestPackage request;
        request.u.close.id = id;
        request.u.close.shutdown = 1;
        request.u.close.opaque = opaque;
        SendRequest_(request, 'K', sizeof(request.u.close));
    }

    void SocketServer::Start(uintptr_t opaque, int id) {
        RequestPackage request;
        request.u.start.id = id;
        request.u.start.opaque = opaque;
        SendRequest_(request, 'S', sizeof(request.u.start));
    }

    int SocketServer::Send(SocketSendBuffer &buffer) {
        int id = buffer.id;
        SocketPtr s = GetSocket(id);
        if (s->GetId() != id || s->GetType() == Socket::Type::INVALID) {
            return SOCKET_NONE;
        }
        // 检查是否可以直接发送
        if (s->CanDirectWrite(id) && s->mutex.try_lock()) {
            // 双重检查
            if (s->CanDirectWrite(id)) {
                SendObject so;
                so.InitFromSendBuffer(buffer);
                ssize_t n;
                if (s->GetProtocol() == SocketProtocol::TCP) {
                    n = SocketApi::Write(s->GetSockFd(), so.buffer.get(), so.sz);
                } else {
                    SockAddress sa;
                    socklen_t sa_sz = s->UdpAddress(s->GetUdpAddress(), sa);
                    if (sa_sz == 0) {
                        fprintf(stderr, "socket-server : set udp (%d) address first.\n", id);
                        s->mutex.unlock();
                        return SOCKET_NONE;
                    }
                    n = SocketApi::SendTo(s->GetSockFd(), so.buffer.get(), so.sz, 0, sa.GetSockAddr(), sa_sz);
                }
                if (n < 0) {
                    // ignore error, let socket thread try again
                    n = 0;
                }
                s->StatWrite(n, time_);
                if (n == so.sz) {
                    // write done
                    s->mutex.unlock();
                    return 0;
                }
                s->SetDwBuffer(std::make_shared<DynamicBuffer>(buffer.buffer, buffer.sz, n));
                poll_->Write(s->GetSockFd(), s.get(), true);
                s->mutex.unlock();
                return 0;
            }
            s->mutex.unlock();
        }
        s->DecSendingRef(id);
        RequestPackage request;
        request.u.send.id = id;
        request.u.send.buffer = static_cast<char *>(buffer.buffer.get());
        request.u.send.sz = buffer.sz;
        SendRequest_(request, 'D', sizeof(request.u.send));
        return E_OK;
    }

    SocketPtr SocketServer::GetSocket(int id) {
        return slot_[HASH_ID(id)];
    }

    int SocketServer::SendLowPriority(SocketSendBuffer &buffer) {
        int id = buffer.id;
        SocketPtr s = GetSocket(id);
        if (s->GetId() != id || s->GetType() == Socket::Type::INVALID) {
            buffer.FreeBuffer();
            return SOCKET_NONE;
        }
        s->DecSendingRef(id);
        RequestPackage request;
        request.u.send.id = id;
        request.u.send.buffer = static_cast<char *>(buffer.buffer.get());
        request.u.send.sz = buffer.sz;
        SendRequest_(request, 'P', sizeof(request.u.send));
        return E_OK;
    }

    static int DoBind(std::string_view host, int port, int protocol, int& family) {
        SockAddressPtr addr;

        if (host.empty()) {
            addr = std::make_shared<SockAddress>(port, false, false);
        } else {
            addr = std::make_shared<SockAddress>(host, port, false);
        }

        bool is_udp = false;
        if (protocol != IPPROTO_TCP) {
            assert(protocol == IPPROTO_UDP);
            is_udp = true;
        }
        family = addr->Family();
        Socket s(SocketApi::Create(addr->Family(), false, is_udp));
        s.SetReuseAddr(true);
        s.BindAddress(*addr);
        return s.GetSockFd();
    }


    static int DoListen(std::string_view host, int port, int backlog) {
        int family = 0;
        int listen_fd = DoBind(host, port, IPPROTO_TCP, family);
        if (listen_fd < 0) {
            return SocketServer::SOCKET_NONE;
        }
        if (SocketApi::Listen(listen_fd, backlog) < 0) {
            SocketApi::Close(listen_fd);
        }
        return listen_fd;
    }


    int SocketServer::Listen(uintptr_t opaque, std::string_view addr, int port, int backlog) {
        int fd = DoListen(addr, port, backlog);
        if (fd < 0) {
            return SOCKET_NONE;
        }
        RequestPackage request;
        int id = ReserveId_();
        if (id < 0) {
            close(fd);
            return id;
        }
        request.u.listen.opaque = opaque;
        request.u.listen.id = id;
        request.u.listen.fd = fd;
        SendRequest_(request, 'L', sizeof(request.u.listen));
        return 0;
    }

    int SocketServer::ReserveId_() {
        for (int i = 0; i < MAX_SOCKET; i++) {
            int id = alloc_id_.fetch_add(1);
            if (id < 0) {
                id = alloc_id_.fetch_and(0x7fffffff);
            }
            SocketPtr s = GetSocket(id);
            if (s->GetType() == Socket::Type::INVALID) {
                if (s->Reserve(id)) {
                    return id;
                } else {
                    // 重试
                    --i;
                }
            }
        }
        return SOCKET_NONE;
    }

    int SocketServer::Connect(uintptr_t opaque, std::string_view addr, int port) {
        RequestPackage request;
        int len = OpenRequest_(request, opaque, addr, port);
        if (len < 0)
            return SOCKET_NONE;
        SendRequest_(request, 'O', sizeof(request.u.open) + len);
        return request.u.open.id;
    }

    int SocketServer::OpenRequest_(RequestPackage &req, uintptr_t opaque, std::string_view addr, int port) {
        int len = addr.size();
        if ((len + sizeof(req.u.open)) >= 256 ) {
            fprintf(stderr, "socket server : invalid addr %s.\n", addr.data());
            return SOCKET_NONE;
        }
        int id = ReserveId_();
        if (id < 0)
            return SOCKET_NONE;
        req.u.open.opaque = opaque;
        req.u.open.id = id;
        req.u.open.port = port;
        memcpy(req.u.open.host, addr.data(), len);
        req.u.open.host[len] = '\0';
        return len;
    }

    int SocketServer::Bind(uintptr_t opaque, int fd) {
        RequestPackage request;
        int id = ReserveId_();
        if (id < 0)
            return SOCKET_NONE;
        request.u.bind.opaque = opaque;
        request.u.bind.id = id;
        request.u.bind.fd = fd;
        SendRequest_(request, 'B', sizeof(request.u.bind));
        return id;
    }

    int SocketServer::NoDelay(int id) {
        RequestPackage request;
        request.u.setopt.id = id;
        request.u.setopt.what = TCP_NODELAY;
        request.u.setopt.value = 1;
        SendRequest_(request, 'T', sizeof(request.u.setopt));
        return SOCKET_NONE;
    }

    // mainloop thread
    static void ForwardMessage(SocketType type, bool padding, SocketMessage &result) {
        TinkSocketMsgPtr sm = std::make_shared<TinkSocketMessage>();


        size_t sz = sizeof(*sm);
        // todo 这里逻辑不对
//        if (result.data) {
//            sz += strlen(static_cast<byte*>(result.data.get()));
//        }
        sm->type = static_cast<int>(type);
        sm->id = result.id;
        sm->ud = result.ud;
        sm->buffer = result.data;

        TinkMessage message;
        message.source = 0;
        message.session = 0;
        message.data = sm;
        message.size = sz | (static_cast<size_t>(PTYPE_SOCKET) << MESSAGE_TYPE_SHIFT);
        HANDLE_STORAGE.PushMessage(result.opaque, message);
    }
    int SocketServer::Poll() {
        SocketMessage result;
        int more = 1;
        int type = Poll_(result, more);
        switch (type) {
            case SOCKET_EXIT:
                return 0;
            case SOCKET_DATA:
                ForwardMessage(SocketType::DATA, false, result);
                break;
            case SOCKET_CLOSE:
                ForwardMessage(SocketType::CLOSE, false, result);
                break;
            case SOCKET_OPEN:
                ForwardMessage(SocketType::CONNECT, true, result);
                break;
            case SOCKET_ERR:
                ForwardMessage(SocketType::ERROR, true, result);
                break;
            case SOCKET_ACCEPT:
                ForwardMessage(SocketType::ACCEPT, true, result);
                break;
            case SOCKET_UDP:
                ForwardMessage(SocketType::UDP, false, result);
                break;
            case SOCKET_WARNING:
                ForwardMessage(SocketType::WARNING, false, result);
                break;
            default:
                spdlog::error("Unknown socket message type {}.",type);
                return SOCKET_NONE;
        }
        if (more) {
            return SOCKET_NONE;
        }
        return 1;
    }

    int SocketServer::StartSocket_(RequestStart *request, SocketMessage &result) {
        int id = request->id;
        result.id = id;
        result.opaque = request->opaque;
        result.ud = 0;
        result.data = nullptr;
        SocketPtr s = GetSocket(id);
        if (s->GetType() == Socket::Type::INVALID || s->GetId() !=id) {
            result.data = StringMessage("invalid socket");
            return SOCKET_ERR;
        }
        if (s->GetType() == Socket::Type::PACCEPT || s->GetType() == Socket::Type::PLISTEN) {
            if (poll_->Add(s->GetSockFd(), s.get())) {
                ForceClose_(*s, result);
                result.data = StringMessage(strerror(errno));
                return SOCKET_ERR;
            }
            s->SetType((s->GetType() == Socket::Type::PACCEPT) ? Socket::Type::CONNECTED : Socket::Type::LISTEN);
            s->SetOpaque(request->opaque);
            result.data = StringMessage("start");
            return SOCKET_OPEN;
        } else if (s->GetType() == Socket::Type::CONNECTED) {
            s->SetOpaque(request->opaque);
            result.data = StringMessage("transfer");
            return SOCKET_OPEN;
        }
        return SOCKET_NONE;
    }

    int SocketServer::BindSocket_(RequestBind *request, SocketMessage &result) {
        int id = request->id;
        result.id = id;
        result.opaque = request->opaque;
        result.ud = 0;
        SocketPtr s = NewSocket_(id, request->fd, SocketProtocol::TCP, request->opaque, true);
        if (!s) {
            result.data = StringMessage("reach socket number limit");
            return SOCKET_ERR;
        }
        SocketApi::NonBlocking(request->fd);
        s->SetType(Socket::Type::BIND);
        result.data = StringMessage("binding");
        return SOCKET_OPEN;
    }

    int SocketServer::ListenSocket_(RequestListen *request, SocketMessage &result) {
        int id = request->id;
        int listen_fd = request->fd;
        SocketPtr s = NewSocket_(id, listen_fd, SocketProtocol::TCP, request->opaque, false);
        if (!s) {
            SocketApi::Close(listen_fd);
            result.opaque = request->opaque;
            result.id = id;
            result.ud = 0;
            result.data = StringMessage("reach socket number limit");
            slot_[HASH_ID(id)]->SetType(Socket::Type::INVALID);
            return SOCKET_ERR;
        }
        s->SetType(Socket::Type::PLISTEN);
        return SOCKET_NONE;
    }

    int SocketServer::CloseSocket_(RequestClose *request, SocketMessage &result) {
        int id = request->id;
        SocketPtr s = GetSocket(id);
        if (s->GetType() == Socket::Type::INVALID || s->GetId() != id) {
            result.id = id;
            result.opaque = request->opaque;
            result.ud = 0;
            result.data = nullptr;
            return SOCKET_CLOSE;
        }
        if (!s->NoMoreSendingData()) {
            int type;
            if (type != -1 && type != SOCKET_WARNING) {
                return type;
            }
        }
        if (request->shutdown || s->NoMoreSendingData()) {
            ForceClose_(*s, result);
            result.id = id;
            result.opaque = request->opaque;
            return SOCKET_CLOSE;
        }
        s->SetType(Socket::Type::HALFCLOSE);
        return SOCKET_NONE;
    }

    int SocketServer::SendBuffer_(Socket &s, SocketMessage &result) {

        if (s.mutex.try_lock()) {
            return SOCKET_NONE;
        }
        if (s.GetDWBuffer()) {
            auto& dw_buffer = s.GetDWBuffer();
            WriteBufferPtr buf = std::make_shared<WriteBuffer>();
            buf->ptr = (reinterpret_cast<byte *>(dw_buffer->GetData().get()) + dw_buffer->GetOffset());
            buf->buffer = dw_buffer->GetData();
            buf->sz = dw_buffer->GetSize();
            s.AddWbSize(buf->sz);
            s.GetHigh().emplace_back(buf);
            s.GetDWBuffer().reset();
        }
        int r = DoSendBuffer_(s, result);
        s.mutex.unlock();
        return r;
    }

    static inline int
    ListUncomplete(WriteBufferList &s) {
        if (s.empty())
            return 0;
        ;
        return (void *)(*s.begin())->ptr != (*s.begin())->buffer.get();
    }

    int SocketServer::SendList_(Socket &s, WriteBufferList &list, SocketMessage &result) {
        if (s.GetProtocol() == SocketProtocol::TCP) {
            return SendListTCP_(s, list, result);
        } else {
            return SendListUDP_(s, list, result);
        }
    }

    int SocketServer::SendListTCP_(Socket &s, WriteBufferList &list, SocketMessage &result) {
        for (auto& tmp : list) {
            for (;;) {
                ssize_t sz = SocketApi::Write(s.GetSockFd(), tmp->ptr, tmp->sz);
                if (sz < 0) {
                    switch (errno) {
                        case EINTR:
                            continue;
                        case AGAIN_WOULDBLOCK:
                            return SOCKET_NONE;
                    }
                    ForceClose_(s, result);
                    return SOCKET_CLOSE;
                }
                s.StatWrite(sz, time_);
                s.SetWbSize(s.GetWbSize()-sz);
                if (sz != tmp->sz) {
                    tmp->ptr += sz;
                    tmp->sz -= sz;
                    return SOCKET_NONE;
                }
                break;
            }
            tmp->buffer.reset();
        }
        list.clear();
        return SOCKET_NONE;
    }

    static void DropUdp(Socket &s, WriteBufferList &list, WriteBufferPtr& tmp) {
        s.SetWbSize(s.GetWbSize()-tmp->sz);
        list.erase(list.begin());
        tmp->buffer.reset();
    }

    int SocketServer::SendListUDP_(Socket &s, WriteBufferList &list, SocketMessage &result) {
        for (auto& tmp : list) {
            SockAddress sa;
            socklen_t sa_sz =  s.UdpAddress(tmp->upd_address, sa);
            if (sa_sz == 0) {
                fprintf(stderr, "socket server : udp (%d) type mismatch.\n", s.GetId());
                DropUdp(s, list, tmp);
                return SOCKET_NONE;
            }
            int err = SocketApi::SendTo(s.GetSockFd(), tmp->ptr, tmp->sz, 0, sa.GetSockAddr(), sa_sz);
            if (err < 0) {
                switch(errno) {
                    case EINTR:
                    case AGAIN_WOULDBLOCK:
                        return SOCKET_NONE;
                }
                fprintf(stderr, "socket server : udp (%d) sendto error %s.\n",s.GetSockFd(), strerror(errno));
                DropUdp(s, list, tmp);
                return SOCKET_NONE;
            }
            s.StatWrite(tmp->sz, time_);
            s.SetWbSize(s.GetWbSize()-tmp->sz);
            tmp->buffer.reset();
        }
        list.clear();
        return SOCKET_NONE;
    }

    int SocketServer::OpenSocket(RequestOpen *request, SocketMessage &result) {
        int id = request->id;
        SocketPtr ns;
        result.opaque = request->opaque;
        result.id = id;
        result.ud = 0;
        result.data = nullptr;
        int status, sock;
        struct addrinfo ai_hints;
        struct addrinfo *ai_list = nullptr;
        struct addrinfo *ai_ptr = nullptr;
        char port[16];
        sprintf(port, "%d", request->port);
        memset(&ai_hints, 0, sizeof( ai_hints ) );
        ai_hints.ai_family = AF_UNSPEC;
        ai_hints.ai_socktype = SOCK_STREAM;
        ai_hints.ai_protocol = IPPROTO_TCP;
        status = getaddrinfo( request->host, port, &ai_hints, &ai_list );
        ScopeGuard sg([&ai_list] {
            freeaddrinfo( ai_list );
        });
        auto _failed = [this, &id] {
            GetSocket(id)->SetType(Socket::Type::INVALID);
            return SOCKET_ERR;
        };
        if ( status != 0 ) {
            result.data = StringMessage(gai_strerror(status));
            return _failed();
        }
        sock = -1;
        for (ai_ptr = ai_list; ai_ptr != nullptr; ai_ptr = ai_ptr->ai_next ) {
            sock = SocketApi::Create( ai_ptr->ai_family, true, false );
            if ( sock < 0 ) {
                continue;
            }
            SocketApi::SetKeepAlive(sock, true);
            status = SocketApi::Connect(sock, ai_ptr->ai_addr);
            if ( status != 0 && errno != EINPROGRESS) {
                SocketApi::Close(sock);
                sock = -1;
                continue;
            }
            break;
        }
        if (sock < 0) {
            result.data = StringMessage(strerror(errno));
            return _failed();
        }
        ns = NewSocket_(id, sock, SocketProtocol::TCP, request->opaque, true);
        if (!ns) {
            SocketApi::Close(sock);
            result.data = StringMessage("reach socket number limit");
            return _failed();
        }

        if (status == 0) {
            ns->SetType(Socket::Type::CONNECTED);
            struct ::sockaddr * addr = ai_ptr->ai_addr;
            SocketApi::ToIp(buffer_.get(), MAX_INFO, ai_ptr->ai_addr);
            result.data = buffer_;

        } else {
            ns->SetType(Socket::Type::CONNECTING);
            poll_->Write(ns->GetSockFd(), ns.get(), true);
        }
        return SOCKET_NONE;
    }

    int SocketServer::SendSocket_(RequestSend *request, SocketMessage &result, int priority, const uint8_t *udp_address) {
        int id = request->id;
        SocketPtr s = GetSocket(id);
        Socket::Type type = s->GetType();
        SendObject so;
        so.Init(request->buffer, request->sz);
        if (type == Socket::Type::INVALID || s->GetId() != id
            || type == Socket::Type::HALFCLOSE
            || type == Socket::Type::PACCEPT) {
            return SOCKET_NONE;
        }
        if (type == Socket::Type::PLISTEN || type == Socket::Type::LISTEN) {
            fprintf(stderr, "socket server: write to listen fd %d.\n", id);
            return SOCKET_NONE;
        }
        if (s->SendBufferEmpty() && type == Socket::Type::CONNECTED) {
            if (s->GetProtocol() == SocketProtocol::TCP) {
                AppendSendBuffer(s, request);
            } else {
                if (udp_address == nullptr) {
                    udp_address = s->GetUdpAddress();
                }
                SockAddress sa;
                socklen_t sa_sz = s->UdpAddress(udp_address, sa);
                if (sa_sz == 0) {
                    fprintf(stderr, "socket-server: udp socket (%d) type mistach.\n", id);
                    return SOCKET_NONE;
                }
                int n = SocketApi::SendTo(s->GetSockFd(), so.buffer.get(), so.sz, 0, sa.GetSockAddr(), sa_sz);
                if (n != so.sz) {
                    AppendSendBufferUdp(s, priority, request, udp_address);
                } else {
                    s->StatWrite(n, time_);
                    return SOCKET_NONE;
                }
            }
            poll_->Write(s->GetSockFd(), s.get(), true);
        } else {
            if (s->GetProtocol() == SocketProtocol::TCP) {
                if (priority == PRIORITY_LOW) {
                    AppendSendBufferLow(s, request);
                } else {
                    AppendSendBuffer(s, request);
                }
            } else {
                if (udp_address == nullptr) {
                    udp_address == s->GetUdpAddress();
                }
                AppendSendBufferUdp(s, priority, request, udp_address);
            }
        }
        int warn_size = s->GetWarnSize();
        int wb_size = s->GetWbSize();
        if (wb_size >= WARNING_SIZE && wb_size >= warn_size) {
            s->SetWarnSize(warn_size == 0 ? WARNING_SIZE * 2 : warn_size * 2);
            result.opaque = s->GetOpaque();
            result.id = s->GetId();
            result.ud = wb_size % 1024 == 0 ? wb_size/1024 : wb_size/1024 + 1;
            result.data = nullptr;
            return SOCKET_WARNING;
        }

        return SOCKET_NONE;
    }

    void SocketServer::DecSendingRef_(int id) {
        SocketPtr s = GetSocket(id);
        s->DecSendingRef(id);
    }

    int SocketServer::SetUdpAddress_(RequestSetUdp *request, SocketMessage &result) {
        int id = request->id;
        SocketPtr s = GetSocket(id);
        if (s->GetType() == Socket::Type::INVALID || s->GetId() !=id) {
            return SOCKET_NONE;
        }
        auto type = static_cast<SocketProtocol>(request->address[0]);
        if (type != s->GetProtocol()) {
            // protocol mismatch
            result.opaque = s->GetOpaque();
            result.id = s->GetId();
            result.ud = 0;
            result.data = StringMessage("protocol mismatch");
            return SOCKET_ERR;
        }
        if (type == SocketProtocol::UDP) {
            memcpy((void *) s->GetUdpAddress(), request->address, 1 + 2 + 4);	// 1 type, 2 port, 4 ipv4
        } else {
            memcpy((void *) s->GetUdpAddress(), request->address, 1+2+16);	// 1 type, 2 port, 16 ipv6
        }
        --s->udp_connecting;
        return SOCKET_NONE;
    }

    void SocketServer::SetOptSocket_(RequestSetOpt *request) {
        int id = request->id;
        SocketPtr s = GetSocket(id);
        if (s->GetType() == Socket::Type::INVALID || s->GetId() !=id) {
            return;
        }
        int v = request->value;
        setsockopt(s->GetSockFd(), IPPROTO_TCP, request->what, &v, sizeof(v));
    }

    void SocketServer::AddUdpSocket_(RequestUdp *udp) {
        int id = udp->id;
        SocketProtocol protocol;
        if (udp->family == AF_INET6) {
            protocol = SocketProtocol::UDPv6;
        } else {
            protocol = SocketProtocol::UDP;
        }
        SocketPtr ns = NewSocket_(id, udp->fd, protocol, udp->opaque, true);
        if (!ns) {
            close(udp->fd);
            GetSocket(id)->SetType(Socket::Type::INVALID);
            return;
        }
        ns->SetType(Socket::Type::CONNECTED);
        memset((void *) ns->GetUdpAddress(), 0, UDP_ADDRESS_SIZE);
    }

    void SocketServer::ClearClosedEvent_(SocketMessage &result, int type) {
        if (type == SOCKET_CLOSE || type == SOCKET_ERR) {
            int id = result.id;
            int i;
            for (i=event_index_; i < event_n_; i++) {
                Event &e = ev_[i];
                Socket *s = static_cast<Socket *>(e.s);
                if (s) {
                    if (s->GetType() == Socket::Type::INVALID && s->GetId() == id) {
                        e.s = nullptr;
                        break;
                    }
                }
            }
        }
    }

    int SocketServer::ReportConnect_(Socket &s, SocketMessage &result) {
        int error;
        socklen_t len = sizeof(error);
        int code = getsockopt(s.GetSockFd(), SOL_SOCKET, SO_ERROR, &error, &len);
        if (code < 0 || error) {
            ForceClose_(s, result);
            if (code >= 0)
                result.data = StringMessage(strerror(errno));
            else
                result.data = StringMessage(strerror(errno));
            return SOCKET_ERR;
        } else {
            s.SetType(Socket::Type::CONNECTED);
            result.opaque = s.GetOpaque();
            result.id = s.GetId();
            result.ud = 0;
            if (s.NoMoreSendingData()) {
                poll_->Write(s.GetSockFd(), &s, false);
            }

            sockaddr_in6 u = SocketApi::GetPeerAddr(s.GetSockFd());
            SocketApi::ToIp(buffer_.get(), MAX_INFO, static_cast<sockaddr*>((void *)&u.sin6_addr));
            result.data = buffer_;
            return SOCKET_OPEN;
        }
    }

    static int GetName(SockAddress &sa, char *buffer, size_t sz) {
        string name = sa.ToIpPort();
        strncpy(buffer, name.c_str(), sz);
        return 1;
    }

    // return 0 when failed, or -1 when file limit
    int SocketServer::ReportAccept_(Socket &s, SocketMessage &result) {
        SockAddress u;
        int client_fd = s.Accept(u);
        if (client_fd < 0) {
            if (errno == EMFILE || errno == ENFILE) {
                result.opaque = s.GetOpaque();
                result.id = s.GetId();
                result.ud = 0;
                result.data = StringMessage(strerror(errno));
                return SOCKET_NONE;
            } else {
                return 0;
            }
        }
        int id = ReserveId_();
        if (id < 0) {
            SocketApi::Close(client_fd);
            return 0;
        }
        SocketApi::NonBlocking(client_fd);
        SocketPtr ns = NewSocket_(id, client_fd, SocketProtocol::TCP, s.GetOpaque(), false);
        if (!ns) {
            SocketApi::Close(client_fd);
        }
        ns->SetKeepAlive(true);

        s.StatRead(1, time_);
        ns->SetType(Socket::Type::PACCEPT);
        result.opaque = s.GetOpaque();
        result.id = s.GetId();
        result.ud = id;
        result.data = nullptr;

        GetName(u, buffer_.get(), MAX_INFO);
        result.data = buffer_;
        return 1;
    }

    int SocketServer::ForwardMessageTcp_(Socket &s, SocketMessage &result) {
        int sz = s.GetReadSize();
        DataPtr buffer(new byte[sz], std::default_delete<byte[]>());
        int n = SocketApi::Read(s.GetSockFd(), buffer.get(), sz);
        if (n < 0) {
            switch(errno) {
                case EINTR:
                    break;
                case AGAIN_WOULDBLOCK:
                    fprintf(stderr, "socket server: EAGAIN capture.\n");
                    break;
                default:
                    // close when error
                    ForceClose_(s, result);
                    result.data = StringMessage(strerror(errno));
                    return SOCKET_ERR;
            }
            return SOCKET_NONE;
        }
        if (n == 0) {
            ForceClose_(s, result);
            return SOCKET_CLOSE;
        }

        if (s.GetType() == Socket::Type::HALFCLOSE) {
            return SOCKET_NONE;
        }

        if (n == sz) {
            s.SetReadSize(sz * 2);
        } else if (sz >= MIN_READ_BUFFER && n*2 < sz) {
            s.SetReadSize(sz / 2);
        }
        result.opaque = s.GetOpaque();
        result.id = s.GetId();
        result.ud = n;
        result.data = buffer;
        return SOCKET_DATA;
    }

    int SocketServer::ForwardMessageUpd_(Socket &s, SocketMessage &result) {
        SockAddress sa;
        socklen_t s_len = sizeof(sa);
        int n = ::recvfrom(s.GetSockFd(), udp_buffer_, MAX_UDP_PACKAGE, 0, const_cast<sockaddr *>(sa.GetSockAddr()), &s_len);
        if (n < 0) {
            switch(errno) {
                case EINTR:
                case AGAIN_WOULDBLOCK:
                    break;
                default:
                    // close when error
                    ForceClose_(s, result);
                    result.data = StringMessage(strerror(errno));
                    return SOCKET_ERR;
            }
            return SOCKET_NONE;
        }
        uint8_t * data;
        if (s_len == sizeof(struct sockaddr_in)) {
            if (s.GetProtocol() != SocketProtocol::UDP)
                return SOCKET_NONE;
            data = new uint8_t[n + 1 + 2 + 4];
            sa.GenUpdAddress(SocketProtocol::UDP, data + n);
        } else {
            if (s.GetProtocol() != SocketProtocol::UDPv6)
                return SOCKET_NONE;
            data = new uint8_t[n + 1 + 2 + 16];
            sa.GenUpdAddress(SocketProtocol::UDPv6, data + n);
        }
        memcpy(data, udp_buffer_, n);

        result.opaque = s.GetOpaque();
        result.id = s.GetId();
        result.ud = n;
        result.data = DataPtr(data, std::default_delete<uint8_t[]>());

        return 0;
    }

    int SocketServer::DoSendBuffer_(Socket &s, SocketMessage &result) {
        assert(!ListUncomplete(s.GetLow()));
        if (SendList_(s, s.GetHigh(), result) == SOCKET_CLOSE) {
            return SOCKET_CLOSE;
        }
        if (s.GetHigh().empty()) {
            if (!s.GetLow().empty()) {
                if (SendList_(s, s.GetLow(), result) == SOCKET_CLOSE) {
                    return SOCKET_CLOSE;
                }

                if (ListUncomplete(s.GetLow())) {
                    s.RaiseUnComplete();
                    return SOCKET_NONE;
                }
                if (!s.GetLow().empty()) {
                    return SOCKET_NONE;
                }
            }
            assert(s.SendBufferEmpty() && s.GetWbSize() == 0);
            poll_->Write(s.GetSockFd(), &s, false);
            if (s.GetType() == Socket::Type::HALFCLOSE) {
                ForceClose_(s, result);
                return SOCKET_CLOSE;
            }
            if (s.GetWarnSize() > 0){
                s.SetWarnSize(0);
                result.opaque = s.GetOpaque();
                result.id = s.GetId();
                result.ud = 0;
                result.data = nullptr;
                return SOCKET_WARNING;
            }
        }
        return SOCKET_NONE;
    }

    int SocketServer::Send(int id, DataPtr buffer, int sz) {
        SocketSendBuffer tmp;
        tmp.Init(id, buffer, sz);
        return Send(tmp);

    }

    SocketServer::SocketServer() : ev_(MAX_EVENT)
    ,buffer_(new byte[MAX_INFO], std::default_delete<byte[]>()) {
    }

}
