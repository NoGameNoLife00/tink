#ifndef TINK_SOCKET_SERVER_H
#define TINK_SOCKET_SERVER_H

#include <cstdint>
#include <list>
#include <atomic>
#include <map>
#include "base/buffer.h"
#include "net/poller.h"
#include "net/socket.h"
#include "common.h"
#include "net/server.h"

namespace tink {
    class Server;

    struct SocketMessage;
    struct SendObject;
    struct RequestStart;
    struct RequestBind;
    struct RequestListen;
    struct RequestClose;
    struct RequestOpen;
    struct RequestSend;
    struct RequestSendUdp;
    struct RequestSetUdp;
    struct RequestSetOpt;
    struct RequestUdp;
    struct RequestPackage;

    using SocketPtr = std::shared_ptr<Socket>;
    class SocketServer : noncopyable {
    public:
        using WriteBufferList = std::list<WriteBufferPtr>;
        using SocketMsgPtr = std::shared_ptr<SocketMessage>;
        using PollerPtr = std::shared_ptr<Poller>;

        enum SocketCode {
            SOCKET_NONE = -1,
            SOCKET_DATA = 0,
            SOCKET_CLOSE = 1,
            SOCKET_OPEN = 2,
            SOCKET_ACCEPT = 3,
            SOCKET_ERR = 4,
            SOCKET_EXIT = 5,
            SOCKET_UDP = 6,
            SOCKET_WARNING = 7,
        };
        static const int MAX_UDP_PACKAGE = 65535;

        SocketServer(std::shared_ptr<Server> server);
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
        enum class SocketType {
            DATA = 1,
            CONNECT = 2,
            CLOSE = 3,
            ACCEPT = 4,
            ERROR = 5,
            UDP = 6,
            WARNING = 7,
        };
        void ForwardMessage(SocketType type, bool padding, SocketMessage &result);
        SocketPtr NewSocket_(int id, int fd, SocketProtocol protocol, uintptr_t opaque, bool add);
        int Poll_(SocketMessage &result, int &more);
        void ForceClose_(Socket &s, SocketMessage &result);
        int HasCmd_();
        int CtrlCmd_(SocketMessage &result);
        int ReserveId_();
        int SendBuffer_(Socket &s, SocketMessage& result);
        int DoSendBuffer_(Socket &s, SocketMessage& result);
        int SendList_(Socket &s, WriteBufferList& list, SocketMessage& result);
        int SendListTCP_(Socket &s, WriteBufferList& list, SocketMessage& result);
        int SendListUDP_(Socket &s, WriteBufferList& list, SocketMessage& result);
        void ClearClosedEvent_(SocketMessage &result, int type);
        int ReportConnect_(Socket &s, SocketMessage &result);
        int ReportAccept_(Socket &s, SocketMessage &result);
        int ForwardMessageTcp_(Socket &s, SocketMessage &result);
        int ForwardMessageUpd_(Socket &s, SocketMessage &result);
        void DecSendingRef_(int id);

        void SendRequest_(RequestPackage &request, byte type, int len) const;
        int OpenRequest_(RequestPackage &req, uintptr_t opaque, std::string_view addr, int port);
        int StartSocket_(RequestStart *request, SocketMessage &result);
        int BindSocket_(RequestBind *request, SocketMessage& result);
        int ListenSocket_(RequestListen *request, SocketMessage& result);
        int CloseSocket_(RequestClose *request, SocketMessage& result);
        int OpenSocket(RequestOpen *request, SocketMessage& result);
        int SendSocket_(RequestSend *request, SocketMessage& result, int priority, const uint8_t *udp_address);
        int SetUdpAddress_(RequestSetUdp *request, SocketMessage& result);
        void SetOptSocket_(RequestSetOpt *request);
        void AddUdpSocket_(RequestUdp *udp);

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
        std::shared_ptr<Server> server_;
    };

}


#endif //TINK_SOCKET_SERVER_H
