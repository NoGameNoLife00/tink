#ifndef TINK_SOCKET_H
#define TINK_SOCKET_H

#include <cstddef>
#include <cstdint>
#include <endian.h>
#include <netinet/tcp.h>
#include <noncopyable.h>
#include <netinet/in.h>
#include <string_util.h>
#include <socket_api.h>
#include <copyable.h>
#include <sock_address.h>
#include <shared_mutex>
#include <buffer.h>
#include <list>
#include <utility>
#include <spdlog/spdlog.h>

#define PROTOCOL_TCP 0
#define PROTOCOL_UDP 1
#define PROTOCOL_UDPv6 2
#define PROTOCOL_UNKNOWN 255

#define UDP_ADDRESS_SIZE 19	// ipv6 128bit + port 16bit + 1 byte type
#define SOCKET_TYPE_INVALID 0
#define SOCKET_TYPE_RESERVE 1
#define SOCKET_TYPE_PLISTEN 2
#define SOCKET_TYPE_LISTEN 3
#define SOCKET_TYPE_CONNECTING 4
#define SOCKET_TYPE_CONNECTED 5
#define SOCKET_TYPE_HALFCLOSE 6
#define SOCKET_TYPE_PACCEPT 7
#define SOCKET_TYPE_BIND 8

#define MAX_SOCKET_P 16
#define MAX_SOCKET (1<<MAX_SOCKET_P)
#define MAX_SOCKET (1<<MAX_SOCKET_P)
#define HASH_ID(id) (((unsigned)id) % MAX_SOCKET)
#define ID_TAG16(id) ((id>>MAX_SOCKET_P) & 0xffff)
#define MIN_READ_BUFFER 64

#define TINK_SOCKET_TYPE_DATA 1
#define TINK_SOCKET_TYPE_CONNECT 2
#define TINK_SOCKET_TYPE_CLOSE 3
#define TINK_SOCKET_TYPE_ACCEPT 4
#define TINK_SOCKET_TYPE_ERROR 5
#define TINK_SOCKET_TYPE_UDP 6
#define TINK_SOCKET_TYPE_WARNING 7
namespace tink {

    struct WriteBuffer {
        DataPtr buffer;
        char *ptr;
        size_t sz;
        uint8_t upd_address[UDP_ADDRESS_SIZE];
    };

    struct SocketStat {
        uint64_t rtime;
        uint64_t wtime;
        uint64_t read;
        uint64_t write;
    };

    typedef std::shared_ptr<WriteBuffer> WriteBufferPtr;

    struct TinkSocketMessage {
        int type;
        int id;
        int ud;
        DataPtr buffer;
        ~TinkSocketMessage() {
            printf("~TinkSocketMessage_() buff:%d", buffer.get());
        }
    };

    typedef std::shared_ptr<TinkSocketMessage> TinkSocketMsgPtr;
    typedef std::list<WriteBufferPtr> WriteBufferList;

    class Socket : noncopyable {
    public:
        explicit Socket(int fd):sock_fd_(fd) {}
        explicit Socket():sock_fd_(0) {}
        int Init(int id, int fd, int protocol, uintptr_t opaque);
        void Destroy();
        void Close() const;
        ~Socket();
        int GetSockFd() const { return sock_fd_; }
        int GetTcpInfo(struct tcp_info* info) const ;
        int GetTcpInfoString(char *buf, int len) const ;
        void BindAddress(const SockAddress& addr) const;
        void Listen() const;
        int Accept(SockAddress &peer_addr) const;
        void ShutDownWrite() const;

        socklen_t UdpAddress(const uint8_t udp_address[UDP_ADDRESS_SIZE], SockAddress& sa) const;
        void SetType(int t) { type_ = t; }
        int GetType() const { return type_; }
        int GetId() const { return id_; }
        void SetId(int id) { id_ = id; }

        bool SendBufferEmpty() { return high.empty() && low.empty(); }

        bool NoMoreSendingData() { return SendBufferEmpty() && dw_buffer_ && (sending_ & 0xffff) == 0; }

        bool CanDirectWrite(int id) { return id_ == id && NoMoreSendingData() && type_ == SOCKET_TYPE_CONNECTED && udp_connecting == 0;}

        uintptr_t GetOpaque() const { return opaque_; }
        void SetOpaque(uintptr_t opaque) { opaque_ = opaque; }

        uint8_t GetProtocol() const {return protocol_;}
        void SetProtocol(uint8_t protocol) { protocol_ = protocol;}

        std::list<WriteBufferPtr>& GetHigh() { return high; }
        std::list<WriteBufferPtr>& GetLow() { return low; }

        DataBufferPtr& GetDWBuffer() { return dw_buffer_; }

        void SetDwBuffer(DataBufferPtr buf) { dw_buffer_ = std::move(buf); }

        void IncSendingRef(int id);

        void DecSendingRef(int id);

        const uint8_t * GetUdpAddress() { return p_.udp_address; }

        int GetReadSize() const { return p_.size; }

        void SetReadSize(int sz) { p_.size = sz; }

        void StatWrite(int n, uint64_t time) {
            stat_.write += n;
            stat_.wtime = time;
        }

        void StatRead(int n, uint64_t time) {
            stat_.read += n;
            stat_.rtime = time;
        }


        int64_t GetWbSize() const { return wb_size_; }
        void SetWbSize(int64_t s) { wb_size_ = s; }
        void AddWbSize(size_t s) { wb_size_ += s; }

        int64_t GetWarnSize() const { return warn_size_; }
        void SetWarnSize(int64_t s) { warn_size_ = s; }
        void RaiseUnComplete();
        bool Reserve(int id);
        void SetTcpNoDelay(bool active) const;
        void SetKeepAlive(bool active) const;
        void SetReuseAddr(bool active) const;
        void SetReusePort(bool active) const;

        std::atomic_uint16_t udp_connecting;
        mutable std::recursive_mutex mutex;
    private:
        int id_{}; // 在socket池的id
        int sock_fd_; // socket文件描述符
        std::atomic_uint8_t type_; // 状态
        uintptr_t opaque_{}; // socket关联的服务地址
        int protocol_{}; // 协议
        uint64_t wb_size_{}; // 发送数据大小
        WriteBufferList high; // 高优先级发送队列
        WriteBufferList low; // 低优先级发送队列
        std::atomic_uint32_t sending_;
        SocketStat stat_{};
        int64_t warn_size_{};
        union {
            int size;
            uint8_t udp_address[UDP_ADDRESS_SIZE];
        } p_{};
        DataBufferPtr dw_buffer_; // 立刻发送缓冲
//        int dw_offset_;
//        DataPtr dw_buffer_;
//        size_t dw_size_;
    };
    typedef std::shared_ptr<Socket> SocketPtr;
}


#endif //TINK_SOCKET_H
