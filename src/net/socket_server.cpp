#include "socket_server.h"
#include "socket.h"
#include <unistd.h>
#include <cassert>
#include <cstring>
#include <error_code.h>

namespace tink {
    void SocketServer::UpdateTime(uint64_t time) {
        time_ = time;
    }

    int SocketServer::Pool(SocketMsgPtr &result, int &more) {
        for (;;) {
            if (checkctrl) {
                if (HasCmd()) {
                    int type = ctrl_cmd(result);
                    if (type != -1) {
                        clear_closed_event(result, type);
                        return type;
                    } else
                        continue;
                }
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

    int SocketServer::CtrlCmd(SocketMsgPtr &result) {
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
                return start_socket((struct request_start *)buffer, result);
            case 'B':
                return bind_socket((struct request_bind *)buffer, result);
            case 'L':
                return listen_socket((struct request_listen *)buffer, result);
            case 'K':
                return close_socket((struct request_close *)buffer, result);
            case 'O':
                return open_socket((struct request_open *)buffer, result);
            case 'X':
                result->opaque = 0;
                result->id = 0;
                result->ud = 0;
                result->data = NULL;
                return SOCKET_EXIT;
            case 'D':
            case 'P': {
                int priority = (type == 'D') ? PRIORITY_HIGH : PRIORITY_LOW;
                struct request_send * request = (struct request_send *) buffer;
                int ret = send_socket(request, result, priority, NULL);
                dec_sending_ref(request->id);
                return ret;
            }
            case 'A': {
                struct request_send_udp * rsu = (struct request_send_udp *)buffer;
                return send_socket(&rsu->send, result, PRIORITY_HIGH, rsu->address);
            }
            case 'C':
                return set_udp_address((struct request_setudp *)buffer, result);
            case 'T':
                setopt_socket((struct request_setopt *)buffer);
                return -1;
            case 'U':
                add_udp_socket((struct request_udp *)buffer);
                return -1;
            default:
                fprintf(stderr, "socket-server: Unknown ctrl %c.\n",type);
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
        return s;;
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

    void SocketServer::FreeWbList(WbList &list) {

    }

    void SocketServer::ForceClose(const SocketPtr& s, SocketMessage &result) {
        result.id = s->GetId();
        result.ud = 0;
        result.data = nullptr;
        result.opaque = s->GetOpaque();
        int type = s->GetType();
        if (type == SOCKET_TYPE_INVALID) {
            return;
        }
        assert(type != SOCKET_TYPE_RESERVE);
        FreeWbList(s->GetHigh());
        FreeWbList(s->GetLow());
        if (type != SOCKET_TYPE_PACCEPT && type != SOCKET_TYPE_PLISTEN) {
            poll_->Del(s->GetSockFd());
        }
        s->mutex.lock();
        if (type != SOCKET_TYPE_BIND) {
            if (close(s->GetSockFd()) < 0) {
                perror("close socket:");
            }
        }
        s->SetType(SOCKET_TYPE_INVALID);
        s->GetDWBuffer().release();
        s->mutex.unlock();
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

    int SocketServer::Send(SocketSendBuffer &buf) {
        int id = buf.id;
        SocketPtr s = GetSocket(id);
        if (s->GetId() != id || s->GetType() == SOCKET_TYPE_INVALID) {
            buf.FreeBuffer();
            return E_FAILED;
        }
        if (s->CanDirectWrite(id) && s->mutex.try_lock()) {
            if (s->CanDirectWrite(id)) {
                SendObject so;
                so.InitFromSendBuffer(buf);
                ssize_t n;
                if (s->GetProtocol() == PROTOCOL_TCP) {
                    n = SocketApi::Write(s->GetSockFd(), so.buffer.get(), so.sz);
                } else {
                    SockAddress sa;
                    socklen_t sa_sz = s->UdpAddress(s->GetUdpAddress(), sa);
                    if (sa_sz == 0) {
                        fprintf(stderr, "socket-server : set udp (%d) address first.\n", id);
                        s->mutex.unlock();
                        buf.FreeBuffer();
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
                    buf.FreeBuffer();
                    return 0;
                }
                buf;
                s->SetDwBuffer()


            }
        }
        return 0;
    }

    SocketPtr SocketServer::GetSocket(int id) {
        return slot[HASH_ID(id)];
    }



}
