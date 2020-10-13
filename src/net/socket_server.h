#ifndef TINK_SOCKET_SERVER_H
#define TINK_SOCKET_SERVER_H

#include <cstdint>
#include <common.h>
#include <map>
#include <buffer.h>
#include <list>
#include <atomic>
#include "poller.h"
#include "socket.h"

#define SOCKET_NONE -1
#define SOCKET_DATA 0
#define SOCKET_CLOSE 1
#define SOCKET_OPEN 2
#define SOCKET_ACCEPT 3
#define SOCKET_ERR 4
#define SOCKET_EXIT 5
#define SOCKET_UDP 6
#define SOCKET_WARNING 7

#define UDP_ADDRESS_SIZE 19	// ipv6 128bit + port 16bit + 1 byte type
#define MAX_INFO 128
#define MAX_EVENT 64
#define MAX_UDP_PACKAGE 65535
namespace tink {

    typedef struct SocketMessage_ {
        int id;
        uintptr_t opaque;
        int ud;
        char * data;
    }SocketMessage;

    typedef struct RequestStart_ {
        int id;
        uintptr_t opaque;
    }RequestStart;
    typedef struct RequestBind_ {
        int id;
        int fd;
        uintptr_t opaque;
    }RequestBind;
    typedef struct RequestListen_ {
        int id;
        int fd;
        uintptr_t opaque;
        char host[1];
    }RequestListen;
    typedef struct RequestClose_ {
        int id;
        int shutdown;
        uintptr_t opaque;
    }RequestClose;
    typedef struct RequestOpen_ {
        int id;
        int port;
        uintptr_t opaque;
        char host[1];
    }RequestOpen;
    typedef struct RequestSend_ {
        int id;
        size_t sz;
        char *buffer;
    }RequestSend;
    typedef struct RequestSendUdp_ {
        RequestSend send;
        uint8_t address[UDP_ADDRESS_SIZE];
    }RequestSendUdp;
    typedef struct RequestSetUdp_ {
        int id;
        uint8_t address[UDP_ADDRESS_SIZE];
    }RequestSetUdp;
    typedef struct RequestSetOpt_ {
        int id;
        int what;
        int value;
    }RequestSetOpt;
    typedef struct RequestUdp_ {
        int id;
        int fd;
        int family;
        uintptr_t opaque;
    }RequestUdp;

    typedef struct RequestPackage_ {
        RequestPackage_() {}

        uint8_t header[8];	// 6 bytes dummy
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
        } u;
        uint8_t dummy[256];
    }RequestPackage;

    typedef struct SendObject_ {
        DataPtr buffer;
        size_t sz;

        void InitFromSendBuffer(SocketSendBuffer &buf) {
            buffer = buf.buffer;
            sz = buf.sz;
        }
        void Init(void * object, size_t size) {
            buffer.reset(object);
            this->sz = size;
        }
    }SendObject;

    typedef std::list<WriteBufferPtr> WriteBufferList;
    typedef std::shared_ptr<SocketMessage> SocketMsgPtr;

    class SocketServer {
    public:
        int Init(uint64_t time);
        void UpdateTime(uint64_t time);

        int Poll();
        void Destroy();
        void FreeWbList(WriteBufferList &list);

        SocketPtr GetSocket(int id);

        void Exit();
        void Close(uintptr_t opaque, int id);
        void Shutdown(uintptr_t opaque, int id);
        void Start(uintptr_t opaque, int id);

        int Send(SocketSendBuffer &buffer);
        int SendLowPriority(SocketSendBuffer &buffer);

        int Listen(uintptr_t opaque, const string &addr, int port, int backlog);
        int Connect(uintptr_t opaque, const string &addr, int port);
        int Bind(uintptr_t opaque, int fd);

        int NoDelay(int id);

    private:
        SocketPtr NewSocket_(int id, int fd, int protocol, uintptr_t opaque, bool add);
        int Poll_(SocketMessage &result, int &more);
        void ForceClose_(Socket &s, SocketMessage &result);
        int HasCmd_();
        int CtrlCmd_(SocketMessage &result);
        void SendRequest_(RequestPackage &request, byte type, int len);
        int ReserveId_();
        int OpenRequest_(RequestPackage &req, uintptr_t opaque, const string &addr, int port);
        int StartSocket_(RequestStart *request, SocketMessage &result);
        int BindSocket_(RequestBind *request, SocketMessage& result);
        int ListenSocket_(RequestListen *request, SocketMessage& result);
        int CloseSocket_(RequestClose *request, SocketMessage& result);
        int OpenSocket(RequestOpen *request, SocketMessage& result);
        int SendBuffer_(Socket &s, SocketMessage& result);
        int DoSendBuffer_(Socket &s, SocketMessage& result);
        int SendList_(Socket &s, WriteBufferList& list, SocketMessage& result);
        int SendListTCP_(Socket &s, WriteBufferList& list, SocketMessage& result);
        int SendListUDP_(Socket &s, WriteBufferList& list, SocketMessage& result);
        int SendSocket_(RequestSend *request, SocketMessage& result, int priority, const uint8_t *udp_address);
        int SetUdpAddress_(RequestSetUdp *request, SocketMessage& result);
        void SetOptSocket_(RequestSetOpt *request);
        void AddUdpSocket_(RequestUdp *udp);
        void ClearClosedEvent_(SocketMessage &result, int type);
        int ReportConnect_(Socket &s, SocketMessage &result);
        int ReportAccept_(Socket &s, SocketMessage &result);
        int ForwardMessageTcp_(Socket &s, SocketMessage &result);
        int ForwardMessageUpd_(Socket &s, SocketMessage &result);
        void DecSendingRef(int id);

        volatile uint64_t time_;
        int recv_ctrl_fd;
        int send_ctrl_fd;
        int check_ctrl_;
        PollerPtr poll_;
        std::atomic_int alloc_id_;
        int event_n_;
        int event_index_;
        fd_set rfds_;
        char buffer_[MAX_INFO];
        uint8_t udp_buffer_[MAX_UDP_PACKAGE];
        EventList ev_;
        std::array<SocketPtr, MAX_SOCKET> slot_;
    };

    typedef std::shared_ptr<SocketServer> SocketServerPtr;
}


#endif //TINK_SOCKET_SERVER_H
