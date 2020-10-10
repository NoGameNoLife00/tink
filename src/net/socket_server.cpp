#include "socket_server.h"
#include "socket.h"
#include <unistd.h>
#include <cassert>
#include <cstring>
#include <error_code.h>
#include <spdlog/spdlog.h>
#include <context_manage.h>
#include <netdb.h>
#include <arpa/inet.h>
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
    void SocketServer::UpdateTime(uint64_t time) {
        time_ = time;
    }

    int SocketServer::Poll_(SocketMessage &result, int &more) {
        for (;;) {
            if (checkctrl) {
                if (HasCmd()) {
                    int type = CtrlCmd(result);
                    if (type != -1) {
                        ClearClosedEvent(result, type);
                        return type;
                    } else
                        continue;
                } else {
                    checkctrl = 0;
                }
            }
            if (event_index == event_n) {
                event_n = poll_->Wait(ev_);
                checkctrl = 1;
                more = 0;
                event_index = 0;
                if (event_n <= 0) {
                    event_n = 0;
                    if (errno == EINTR) {
                        continue;
                    }
                    return -1;
                }
            }
            Event &e = ev_[event_index++];
            Socket *s = static_cast<Socket *>(e.s);
            if (s == nullptr) {
                continue;
            }
            switch (s->GetType()) {
                case SOCKET_TYPE_CONNECTING:
                    return ReportConnect(*s, result);
                case SOCKET_TYPE_LISTEN: {
                    int ok = ReportAccept(*s, result);
                    if (ok > 0) {
                        return SOCKET_ACCEPT;
                    } if (ok < 0 ) {
                        return SOCKET_ERR;
                    }
                    // when ok == 0, retry
                    break;
                }
                case SOCKET_TYPE_INVALID:
                    fprintf(stderr, "socket server: invalid socket\n");
                break;
                default:
                    if (e.read) {
                        int type;
                        if (s->GetProtocol() == PROTOCOL_TCP) {
                            type = forward_message_tcp(*s, result);
                        } else {
                            type = forward_message_udp(ss, s, &l, result);
                            if (type == SOCKET_UDP) {
                                // try read again
                                --ss->event_index;
                                return SOCKET_UDP;
                            }
                        }
                        if (e->write && type != SOCKET_CLOSE && type != SOCKET_ERR) {
                            // Try to dispatch write message next step if write flag set.
                            e->read = false;
                            --ss->event_index;
                        }
                        if (type == -1)
                            break;
                        return type;
                    }
                if (e->write) {
                    int type = send_buffer(ss, s, &l, result);
                    if (type == -1)
                        break;
                    return type;
                }
                if (e->error) {
                    // close when error
                    int error;
                    socklen_t len = sizeof(error);
                    int code = getsockopt(s->fd, SOL_SOCKET, SO_ERROR, &error, &len);
                    const char * err = NULL;
                    if (code < 0) {
                        err = strerror(errno);
                    } else if (error != 0) {
                        err = strerror(error);
                    } else {
                        err = "Unknown error";
                    }
                    force_close(ss, s, &l, result);
                    result->data = (char *)err;
                    return SOCKET_ERR;
                }
                if(e->eof) {
                    force_close(ss, s, &l, result);
                    return SOCKET_CLOSE;
                }
                break;
            }
        }

        return 0;
    }

    int SocketServer::HasCmd() {
        struct timeval tv = {0,0};
        int retval;
        FD_SET(recvctrl_fd, &rfds);

        retval = select(recvctrl_fd+1, &rfds, NULL, NULL, &tv);
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

    int SocketServer::CtrlCmd(SocketMessage &result) {
        int fd = recvctrl_fd;
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
                RequestSend * request = (RequestSend*) buffer;
                int ret = SendSocket_(request, result, priority, nullptr);
                DecSendingRef(request->id);
                return ret;
            }
            case 'A': {
                RequestSendUdp * rsu = (RequestSendUdp *)buffer;
                return SendSocket_(&rsu->send, result, PRIORITY_HIGH, rsu->address);
            }
            case 'C':
                return SetUdpAddress_((RequestSetUdp *) buffer, result);
            case 'T':
                SetOptSocket_((RequestSetOpt *)buffer);
                return -1;
            case 'U':
                AddUdpSocket_((RequestUdp *) buffer);
                return -1;
            default:
                fprintf(stderr, "socket server: Unknown ctrl %c.\n",type);
                return -1;
        };

        return -1;
    }

    int SocketServer::StartSocket(RequestStart *request, SocketMsgPtr result) {
        int id = request->id;
        result->id = id;
        result->opaque = request->opaque;
        result->ud = 0;
        result->data = nullptr;

        return 0;
    }

    int SocketServer::Init(uint64_t time) {
        int fd[2];
        PollerPtr poll(Poller::NewDefaultPoller());
        if (poll->Invalid()) {
            fprintf(stderr, "socket server: create event pool failed.\n");
            return E_FAILED;
        }
        if (pipe(fd)) {
            fprintf(stderr, "socket server: create socket pair failed.\n");
            return E_FAILED;
        }
        if (poll->Add(fd[0], nullptr)) {
            fprintf(stderr, "socket server: can't add server fd to event pool.\n");
            close(fd[0]);
            close(fd[1]);
            return E_FAILED;
        }

        time_ = time;
        poll_ = poll;
        recvctrl_fd = fd[0];
        sendctrl_fd = fd[1];
        checkctrl = 1;

        alloc_id = 0;
        event_n = 0;
        event_index = 0;

        for (int i = 0; i < slot.max_size(); i++) {
            slot[i] = std::make_shared<Socket>();
            slot[i]->SetType(SOCKET_TYPE_INVALID);
        }

        FD_ZERO(&rfds);
        assert(recvctrl_fd < FD_SETSIZE);
        return 0;
    }

    SocketServer::SocketServer() {
    }

    SocketPtr SocketServer::NewSocket_(int id, int fd, int protocol, uintptr_t opaque, bool add) {
        SocketPtr s = GetSocket(id);
        assert(s->GetType() == SOCKET_TYPE_RESERVE);

        if (add) {
            if (poll_->Add(fd, &s)) {
                s->SetType(SOCKET_TYPE_INVALID);
                return nullptr;
            }
        }
        s->Init(id, fd, protocol, opaque);
        return s;
    }

    void SocketServer::Destroy() {
        SocketMessage dummy;
        for (auto s : slot) {
            if (s->GetType() != SOCKET_TYPE_RESERVE) {
                s->Destroy();
            }
        }
        close(sendctrl_fd);
        close(recvctrl_fd);
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


    void SocketServer::ForceClose(Socket &s, SocketMessage &result) {
        result.id = s.GetId();
        result.ud = 0;
        result.data = nullptr;
        result.opaque = s.GetOpaque();
        int type = s.GetType();
        if (type == SOCKET_TYPE_INVALID) {
            return;
        }
        assert(type != SOCKET_TYPE_RESERVE);
        FreeWbList(s.GetHigh());
        FreeWbList(s.GetLow());
        if (type != SOCKET_TYPE_PACCEPT && type != SOCKET_TYPE_PLISTEN) {
            poll_->Del(s.GetSockFd());
        }
        s.mutex.lock();
        if (type != SOCKET_TYPE_BIND) {
            if (close(s.GetSockFd()) < 0) {
                perror("close socket:");
            }
        }
        s.SetType(SOCKET_TYPE_INVALID);
        s.GetDWBuffer().reset();
        s.mutex.unlock();
    }

    void SocketServer::Exit() {
        RequestPackage request;
        SendRequest(request, 'x', 0);
    }

    void SocketServer::SendRequest(RequestPackage &request, char type, int len) {
        request.header[6] = (uint8_t)type;
        request.header[7] = (uint8_t)len;
        for (;;) {
            ssize_t n = write(sendctrl_fd, &request.header[6], len+2);
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
        SendRequest(request, 'K', sizeof(request.u.close));
    }

    void SocketServer::Shutdown(uintptr_t opaque, int id) {
        RequestPackage request;
        request.u.close.id = id;
        request.u.close.shutdown = 1;
        request.u.close.opaque = opaque;
        SendRequest(request, 'K', sizeof(request.u.close));
    }

    void SocketServer::Start(uintptr_t opaque, int id) {
        RequestPackage request;
        request.u.start.id = id;
        request.u.start.opaque = opaque;
        SendRequest(request, 'S', sizeof(request.u.start));
    }

    int SocketServer::Send(SocketSendBuffer &buffer) {
        int id = buffer.id;
        SocketPtr s = GetSocket(id);
        if (s->GetId() != id || s->GetType() == SOCKET_TYPE_INVALID) {
            buffer.FreeBuffer();
            return E_FAILED;
        }
        if (s->CanDirectWrite(id) && s->mutex.try_lock()) {
            if (s->CanDirectWrite(id)) {
                SendObject so;
                so.InitFromSendBuffer(buffer);
                ssize_t n;
                if (s->GetProtocol() == PROTOCOL_TCP) {
                    n = SocketApi::Write(s->GetSockFd(), so.buffer.get(), so.sz);
                } else {
                    SockAddress sa;
                    socklen_t sa_sz = s->UdpAddress(s->GetUdpAddress(), sa);
                    if (sa_sz == 0) {
                        fprintf(stderr, "socket-server : set udp (%d) address first.\n", id);
                        s->mutex.unlock();
                        return E_FAILED;
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
                s->SetDwBuffer(std::make_shared<DataBuffer>(buffer.buffer, buffer.sz, n));
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
        SendRequest(request, 'D', sizeof(request.u.send));
        return E_OK;
    }

    SocketPtr SocketServer::GetSocket(int id) {
        return slot[HASH_ID(id)];
    }

    int SocketServer::SendLowPriority(SocketSendBuffer &buffer) {
        int id = buffer.id;
        SocketPtr s = GetSocket(id);
        if (s->GetId() != id || s->GetType() == SOCKET_TYPE_INVALID) {
            buffer.FreeBuffer();
            return E_FAILED;
        }
        s->DecSendingRef(id);
        RequestPackage request;
        request.u.send.id = id;
        request.u.send.buffer = static_cast<char *>(buffer.buffer.get());
        request.u.send.sz = buffer.sz;
        SendRequest(request, 'P', sizeof(request.u.send));
        return E_OK;
    }

    static int DoBind(StringArg host, int port, int protocol, int& family) {
        SockAddressPtr addr;

        if (host.c_str() == nullptr || host.c_str()[0] == 0 ) {
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


    static int DoListen(StringArg host, int port, int backlog) {
        int family = 0;
        int listen_fd = DoBind(host, port, IPPROTO_TCP, family);
        if (listen_fd < 0) {
            return E_FAILED;
        }
        if (SocketApi::Listen(listen_fd, backlog) < 0) {
            SocketApi::Close(listen_fd);
        }
        return listen_fd;
    }


    int SocketServer::Listen(uintptr_t opaque, const string &addr, int port, int backlog) {
        int fd = DoListen(addr, port, backlog);
        if (fd < 0) {
            return E_FAILED;
        }
        RequestPackage request;
        int id = ReserveId();
        if (id < 0) {
            close(fd);
            return id;
        }
        request.u.listen.opaque = opaque;
        request.u.listen.id = id;
        request.u.listen.fd = fd;
        SendRequest(request, 'L', sizeof(request.u.listen));
        return 0;
    }

    int SocketServer::ReserveId() {
        for (int i = 0; i < MAX_SOCKET; i++) {
            int id = alloc_id.fetch_add(1);
            if (id < 0) {
                id = alloc_id.fetch_and(0x7fffffff);
            }
            SocketPtr s = GetSocket(id);
            if (s->GetType() == SOCKET_TYPE_INVALID) {
                if (!s->Reserve(id)) {
                    --i;
                }
            }
        }
        return E_FAILED;
    }

    int SocketServer::Connect(uintptr_t opaque, const string &addr, int port) {
        RequestPackage request;
        int len = OpenRequest(request, opaque, addr, port);
        if (len < 0)
            return -1;
        SendRequest(request, 'O', sizeof(request.u.open) + len);
        return request.u.open.id;
    }

    int SocketServer::OpenRequest(RequestPackage &req, uintptr_t opaque, const string &addr, int port) {
        int len = addr.size();
        if ((len + sizeof(req.u.open)) >= 256 ) {
            fprintf(stderr, "socket server : Invalid addr %s.\n", addr.c_str());
            return E_FAILED;
        }
        int id = ReserveId();
        if (id < 0)
            return E_FAILED;
        req.u.open.opaque = opaque;
        req.u.open.id = id;
        req.u.open.port = port;
        memcpy(req.u.open.host, addr.c_str(), len);
        req.u.open.host[len] = '\0';
        return len;
    }

    int SocketServer::Bind(uintptr_t opaque, int fd) {
        RequestPackage request;
        int id = ReserveId();
        if (id < 0)
            return -1;
        request.u.bind.opaque = opaque;
        request.u.bind.id = id;
        request.u.bind.fd = fd;
        SendRequest(request, 'B', sizeof(request.u.bind));
        return id;
    }

    int SocketServer::NoDelay(int id) {
        RequestPackage request;
        request.u.setopt.id = id;
        request.u.setopt.what = TCP_NODELAY;
        request.u.setopt.value = 1;
        SendRequest(request, 'T', sizeof(request.u.setopt));
    }

    // mainloop thread
    static void ForwardMessage(int type, bool padding, SocketMessage &result) {
        TSocketMsgPtr sm = std::make_shared<TSocketMessage>();

        // todo 这里逻辑不对
        size_t sz = sizeof(*sm);
        if (result.data) {
            sz += strlen(result.data);
        }
        sm->type = type;
        sm->id = result.id;
        sm->ud = result.ud;
        sm->buffer.reset(result.data);

        Message message;
        message.source = 0;
        message.session = 0;
        message.data = sm;
        message.size = sz | (static_cast<size_t>(PTYPE_SOCKET) << MESSAGE_TYPE_SHIFT);
        ContextMngInstance.PushMessage(result.opaque, message);
    }
    int SocketServer::Poll() {
        SocketMessage result;
        int more = 1;
        int type = Poll_(result, more);
        switch (type) {
            case SOCKET_EXIT:
                return 0;
            case SOCKET_DATA:
                ForwardMessage(TINK_SOCKET_TYPE_DATA, false, result);
                break;
            case SOCKET_CLOSE:
                ForwardMessage(TINK_SOCKET_TYPE_CLOSE, false, result);
                break;
            case SOCKET_OPEN:
                ForwardMessage(TINK_SOCKET_TYPE_CONNECT, true, result);
                break;
            case SOCKET_ERR:
                ForwardMessage(TINK_SOCKET_TYPE_ERROR, true, result);
                break;
            case SOCKET_ACCEPT:
                ForwardMessage(TINK_SOCKET_TYPE_ACCEPT, true, result);
                break;
            case SOCKET_UDP:
                ForwardMessage(TINK_SOCKET_TYPE_UDP, false, result);
                break;
            case SOCKET_WARNING:
                ForwardMessage(TINK_SOCKET_TYPE_WARNING, false, result);
                break;
            default:
                spdlog::error("Unknown socket message type {}.",type);
                return -1;
        }
        if (more) {
            return -1;
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
        if (s->GetType() == SOCKET_TYPE_INVALID || s->GetId() !=id) {
            result.data = "invalid socket";
            return SOCKET_ERR;
        }
        if (s->GetType() == SOCKET_TYPE_PACCEPT || s->GetType() == SOCKET_TYPE_PLISTEN) {
            if (poll_->Add(s->GetSockFd(), s.get())) {
                ForceClose(*s, result);
                result.data = strerror(errno);
                return SOCKET_ERR;
            }
            s->SetType((s->GetType() == SOCKET_TYPE_PACCEPT) ? SOCKET_TYPE_CONNECTED : SOCKET_TYPE_LISTEN);
            s->SetOpaque(request->opaque);
            result.data = "start";
            return SOCKET_OPEN;
        } else if (s->GetType() == SOCKET_TYPE_CONNECTED) {
            s->SetOpaque(request->opaque);
            result.data = "transfer";
            return SOCKET_OPEN;
        }
        return E_FAILED;
    }

    int SocketServer::BindSocket_(RequestBind *request, SocketMessage &result) {
        int id = request->id;
        result.id = id;
        result.opaque = request->opaque;
        result.ud = 0;
        SocketPtr s = NewSocket_(id, request->fd, PROTOCOL_TCP, request->opaque, true);
        if (!s) {
            result.data = "reach socket number limit";
            return SOCKET_ERR;
        }
        SocketApi::NonBlocking(request->fd);
        s->SetType(SOCKET_TYPE_BIND);
        result.data = "binding";
        return SOCKET_OPEN;
    }

    int SocketServer::ListenSocket_(RequestListen *request, SocketMessage &result) {
        int id = request->id;
        int listen_fd = request->fd;
        SocketPtr s = NewSocket_(id, listen_fd, PROTOCOL_TCP, request->opaque, false);
        if (!s) {
            SocketApi::Close(listen_fd);
            result.opaque = request->opaque;
            result.id = id;
            result.ud = 0;
            result.data = "reach socket number limit";
            slot[HASH_ID(id)]->SetType(SOCKET_TYPE_INVALID);
            return SOCKET_ERR;
        }
        s->SetType(SOCKET_TYPE_PLISTEN);
        return E_FAILED;
    }

    int SocketServer::CloseSocket_(RequestClose *request, SocketMessage &result) {
        int id = request->id;
        SocketPtr s = GetSocket(id);
        if (s->GetType() == SOCKET_TYPE_INVALID || s->GetId() != id) {
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
            ForceClose(*s, result);
            result.id = id;
            result.opaque = request->opaque;
            return SOCKET_CLOSE;
        }
        s->SetType(SOCKET_TYPE_HALFCLOSE);
        return E_FAILED;
    }

    int SocketServer::SendBuffer_(SocketPtr s, SocketMessage &result) {
        if (s->mutex.try_lock()) {
            return E_FAILED;
        }
        if (s->GetDWBuffer()) {
            auto& dw_buffer = s->GetDWBuffer();
            WriteBufferPtr buf = std::make_shared<WriteBuffer>();
            buf->userobj = false;
            buf->ptr = (reinterpret_cast<byte *>(dw_buffer->GetData().get()) + dw_buffer->GetOffset());
            buf->buffer = dw_buffer->GetData();
            buf->sz = dw_buffer->GetSize();
            s->AddWbSize(buf->sz);
            s->GetHigh().emplace_back(buf);
            s->GetDWBuffer().reset();
        }

    }

    static inline int
    ListUncomplete(WriteBufferList &s) {
        if (s.empty())
            return 0;
        ;
        return (void *)(*s.begin())->ptr != (*s.begin())->buffer.get();
    }
    int SocketServer::DoSendBuffer_(SocketPtr s, SocketMessage &result) {
        assert(!ListUncomplete(s->GetLow()));

        return 0;
    }

    int SocketServer::SendList_(SocketPtr s, WriteBufferList &list, SocketMessage &result) {
        if (s->GetProtocol() == PROTOCOL_TCP) {
            return SendListTCP_(s, list, result);
        } else {
            return SendListUDP_(s, list, result);
        }
    }

    int SocketServer::SendListTCP_(SocketPtr s, WriteBufferList &list, SocketMessage &result) {
        for (auto& tmp : list) {
            for (;;) {
                ssize_t sz = SocketApi::Write(s->GetSockFd(), tmp->ptr, tmp->sz);
                if (sz < 0) {
                    switch (errno) {
                        case EINTR:
                            continue;
                        case AGAIN_WOULDBLOCK:
                            return E_FAILED;
                    }
                    ForceClose(*s, result);
                    return SOCKET_CLOSE;
                }
                s->StatWrite(sz, time_);
                s->SetWbSize(s->GetWbSize()-sz);
                if (sz != tmp->sz) {
                    tmp->ptr += sz;
                    tmp->sz -= sz;
                    return E_FAILED;
                }
                break;
            }
            tmp->buffer.reset();
        }
        list.clear();
        return E_FAILED;
    }

    static void DropUdp(SocketPtr &s, WriteBufferList &list, WriteBufferPtr& tmp) {
        s->SetWbSize(s->GetWbSize()-tmp->sz);
        list.erase(list.begin());
        tmp->buffer.reset();
    }

    int SocketServer::SendListUDP_(SocketPtr s, WriteBufferList &list, SocketMessage &result) {
        for (auto& tmp : list) {
            SockAddress sa;
            socklen_t sa_sz =  s->UdpAddress(tmp->upd_address, sa);
            if (sa_sz == 0) {
                fprintf(stderr, "socket server : udp (%d) type mismatch.\n", s->GetId());
                DropUdp(s, list, tmp);
                return E_FAILED;
            }
            int err = SocketApi::SendTo(s->GetSockFd(), tmp->ptr, tmp->sz, 0, sa.GetSockAddr(), sa_sz);
            if (err < 0) {
                switch(errno) {
                    case EINTR:
                    case AGAIN_WOULDBLOCK:
                        return E_FAILED;
                }
                fprintf(stderr, "socket server : udp (%d) sendto error %s.\n",s->GetSockFd(), strerror(errno));
                DropUdp(s, list, tmp);
                return -1;
            }
            s->StatWrite(tmp->sz, time_);
            s->SetWbSize(s->GetWbSize()-tmp->sz);
            tmp->buffer.reset();
        }
        list.clear();
        return E_FAILED;
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
        struct addrinfo *ai_list = NULL;
        struct addrinfo *ai_ptr = NULL;
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
            GetSocket(id)->SetType(SOCKET_TYPE_INVALID);
            return SOCKET_ERR;
        };
        if ( status != 0 ) {
            result.data = const_cast<char *>(gai_strerror(status));
            return _failed();
        }
        sock = -1;
        for (ai_ptr = ai_list; ai_ptr != NULL; ai_ptr = ai_ptr->ai_next ) {
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
            result.data = strerror(errno);
            return _failed();
        }
        ns = NewSocket_(id, sock, PROTOCOL_TCP, request->opaque, true);
        if (!ns) {
            SocketApi::Close(sock);
            result.data = "reach socket number limit";
            return _failed();
        }

        if (status == 0) {
            ns->SetType(SOCKET_TYPE_CONNECTED);
            struct ::sockaddr * addr = ai_ptr->ai_addr;
            SocketApi::ToIp(buffer_, sizeof(buffer_), ai_ptr->ai_addr);
            result.data = buffer_;

        } else {
            ns->SetType(SOCKET_TYPE_CONNECTING);
            poll_->Write(ns->GetSockFd(), ns.get(), true);
        }
        return E_FAILED;
    }

    int SocketServer::SendSocket_(RequestSend *request, SocketMessage &result, int priority, const uint8_t *udp_address) {
        int id = request->id;
        SocketPtr s = GetSocket(id);
        int type = s->GetType();
        SendObject so;
        so.Init(request->buffer, request->sz);
        if (type == SOCKET_TYPE_INVALID || s->GetId() != id
            || type == SOCKET_TYPE_HALFCLOSE
            || type == SOCKET_TYPE_PACCEPT) {
            return -1;
        }
        if (type == SOCKET_TYPE_PLISTEN || type == SOCKET_TYPE_LISTEN) {
            fprintf(stderr, "socket server: write to listen fd %d.\n", id);
            return -1;
        }
        if (s->SendBufferEmpty() && type == SOCKET_TYPE_CONNECTED) {
            if (s->GetProtocol() == PROTOCOL_TCP) {
                AppendSendBuffer(s, request);
            } else {
                if (udp_address == nullptr) {
                    udp_address = s->GetUdpAddress();
                }
                SockAddress sa;
                socklen_t sa_sz = s->UdpAddress(udp_address, sa);
                if (sa_sz == 0) {
                    fprintf(stderr, "socket-server: udp socket (%d) type mistach.\n", id);
                    return -1;
                }
                int n = SocketApi::SendTo(s->GetSockFd(), so.buffer.get(), so.sz, 0, sa.GetSockAddr(), sa_sz);
                if (n != so.sz) {
                    AppendSendBufferUdp(s, priority, request, udp_address);
                } else {
                    s->StatWrite(n, time_);
                    return -1;
                }
            }
            poll_->Write(s->GetSockFd(), s.get(), true);
        } else {
            if (s->GetProtocol() == PROTOCOL_TCP) {
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

        return -1;
    }

    void SocketServer::DecSendingRef(int id) {
        SocketPtr s = GetSocket(id);
        s->DecSendingRef(id);
    }

    int SocketServer::SetUdpAddress_(RequestSetUdp *request, SocketMessage &result) {
        int id = request->id;
        SocketPtr s = GetSocket(id);
        if (s->GetType() == SOCKET_TYPE_INVALID || s->GetId() !=id) {
            return -1;
        }
        int type = request->address[0];
        if (type != s->GetProtocol()) {
            // protocol mismatch
            result.opaque = s->GetOpaque();
            result.id = s->GetId();
            result.ud = 0;
            result.data = "protocol mismatch";
            return SOCKET_ERR;
        }
        if (type == PROTOCOL_UDP) {
            memcpy((void *) s->GetUdpAddress(), request->address, 1 + 2 + 4);	// 1 type, 2 port, 4 ipv4
        } else {
            memcpy((void *) s->GetUdpAddress(), request->address, 1+2+16);	// 1 type, 2 port, 16 ipv6
        }
        --s->udp_connecting;
        return -1;
    }

    void SocketServer::SetOptSocket_(RequestSetOpt *request) {
        int id = request->id;
        SocketPtr s = GetSocket(id);
        if (s->GetType() == SOCKET_TYPE_INVALID || s->GetId() !=id) {
            return;
        }
        int v = request->value;
        setsockopt(s->GetSockFd(), IPPROTO_TCP, request->what, &v, sizeof(v));
    }

    void SocketServer::AddUdpSocket_(RequestUdp *udp) {
        int id = udp->id;
        int protocol;
        if (udp->family == AF_INET6) {
            protocol = PROTOCOL_UDPv6;
        } else {
            protocol = PROTOCOL_UDP;
        }
        SocketPtr ns = NewSocket_(id, udp->fd, protocol, udp->opaque, true);
        if (!ns) {
            close(udp->fd);
            GetSocket(id)->SetType(SOCKET_TYPE_INVALID);
            return;
        }
        ns->SetType(SOCKET_TYPE_CONNECTED);
        memset((void *) ns->GetUdpAddress(), 0, UDP_ADDRESS_SIZE);
    }

    void SocketServer::ClearClosedEvent(SocketMessage &result, int type) {
        if (type == SOCKET_CLOSE || type == SOCKET_ERR) {
            int id = result.id;
            int i;
            for (i=event_index; i<event_n; i++) {
                Event &e = ev_[i];
                Socket *s = static_cast<Socket *>(e.s);
                if (s) {
                    if (s->GetType() == SOCKET_TYPE_INVALID && s->GetId() == id) {
                        e.s = nullptr;
                        break;
                    }
                }
            }
        }
    }

    int SocketServer::ReportConnect(Socket &s, SocketMessage &result) {
        int error;
        socklen_t len = sizeof(error);
        int code = getsockopt(s.GetSockFd(), SOL_SOCKET, SO_ERROR, &error, &len);
        if (code < 0 || error) {
            ForceClose(s, result);
            if (code >= 0)
                result.data = strerror(error);
            else
                result.data = strerror(errno);
            return SOCKET_ERR;
        } else {
            s.SetType(SOCKET_TYPE_CONNECTED);
            result.opaque = s.GetOpaque();
            result.id = s.GetId();
            result.ud = 0;
            if (s.NoMoreSendingData()) {
                poll_->Write(s.GetSockFd(), &s, false);
            }

            sockaddr_in6 u = SocketApi::GetPeerAddr(s.GetSockFd());
            SocketApi::ToIp(buffer_, sizeof(buffer_), static_cast<sockaddr*>((void *)&u.sin6_addr));
            result.data = buffer_;
            return SOCKET_OPEN;
        }
    }

    static int GetName(SockAddress &sa, char *buffer, size_t sz) {
        string name = sa.ToIpPort();
        strcpy(buffer, name.c_str());
        return 1;
    }

    // return 0 when failed, or -1 when file limit
    int SocketServer::ReportAccept(Socket &s, SocketMessage &result) {
        SockAddress u;
        int client_fd = s.Accept(u);
        if (client_fd < 0) {
            if (errno == EMFILE || errno == ENFILE) {
                result.opaque = s.GetOpaque();
                result.id = s.GetId();
                result.ud = 0;
                result.data = strerror(errno);
                return -1;
            } else {
                return 0;
            }
        }
        int id = ReserveId();
        if (id < 0) {
            SocketApi::Close(client_fd);
            return 0;
        }
        SocketApi::NonBlocking(client_fd);
        SocketPtr ns = NewSocket_(id, client_fd, PROTOCOL_TCP, s.GetOpaque(), false);
        if (!ns) {
            SocketApi::Close(client_fd);
        }
        ns->SetKeepAlive(true);

        s.StatRead(1, time_);
        ns->SetType(SOCKET_TYPE_PACCEPT);
        result.opaque = s.GetOpaque();
        result.id = s.GetId();
        result.ud = id;
        result.data = nullptr;

        GetName(u, buffer_, sizeof(buffer_));
        result.data = buffer_;
        return 1;
    }

}
