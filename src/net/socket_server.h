#ifndef TINK_SOCKET_SERVER_H
#define TINK_SOCKET_SERVER_H

#include <cstdint>
#include <common.h>
#include <map>
#include <buffer.h>
#include <list>
#include <atomic>
#include <poller.h>
#include <socket.h>

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
#define MAX_UDP_PACKAGE 65535

#define SOCKET_SERVER tink::Singleton<tink::SocketServer>::GetInstance()

namespace tink {

    struct SocketMessage {
        int id;
        uintptr_t opaque;
        int ud;
        DataPtr data;
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

    typedef std::list<WriteBufferPtr> WriteBufferList;
    typedef std::shared_ptr<SocketMessage> SocketMsgPtr;

    class SocketServer {
    public:
        SocketServer();
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
        int Send(int id, DataPtr buffer, int sz);
        int SendLowPriority(SocketSendBuffer &buffer);
        int Listen(uintptr_t opaque, std::string_view addr, int port, int backlog);
        int Connect(uintptr_t opaque, std::string_view addr, int port);
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
        int OpenRequest_(RequestPackage &req, uintptr_t opaque, std::string_view addr, int port);
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
        int recv_ctrl_fd; // 接受管道
        int send_ctrl_fd; // 发送管道
        int check_ctrl_; // 管道是否有数据
        PollerPtr poll_; // poll实例
        std::atomic_int alloc_id_; // 已经分配的socket池索引
        int event_n_; // 当前poll事件数
        int event_index_; // 下个未处理的poll事件索引
        fd_set rfds_;
        BytePtr buffer_;
        uint8_t udp_buffer_[MAX_UDP_PACKAGE];
        EventList ev_; // poll事件列表
        std::array<SocketPtr, MAX_SOCKET> slot_; // socket池
    };

    typedef std::shared_ptr<SocketServer> SocketServerPtr;
}


#endif //TINK_SOCKET_SERVER_H
