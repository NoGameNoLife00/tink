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

namespace tink {
    const int UDP_ADDRESS_SIZE = 19;	// ipv6 128bit + port 16bit + 1 byte type
    const int MIN_READ_BUFFER = 64;
    const int MAX_SOCKET_P = 16;
    const int MAX_SOCKET = (1<<MAX_SOCKET_P);

    inline uint32_t HASH_ID(int id) { return ((unsigned)id) % MAX_SOCKET; }
    inline uint32_t ID_TAG16(int id) { return (id>>MAX_SOCKET_P) & 0xffff; }

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
//        ~TinkSocketMessage() {
//            printf("~TinkSocketMessage_() buff:%d", buffer.use_count());
//        }
        enum Type {
            DATA = 1,
            CONNECT = 2,
            CLOSE = 3,
            ACCEPT = 4,
            ERROR = 5,
            UDP = 6,
            WARNING = 7,
        };
    };

    typedef std::shared_ptr<TinkSocketMessage> TinkSocketMsgPtr;
    typedef std::list<WriteBufferPtr> WriteBufferList;

    class Socket : noncopyable {
    public:
        enum class Type {
            INVALID = 0, // 空闲
            RESERVE = 1, // 已占用
            PLISTEN = 2, // 等待监听(listen socket使用)
            LISTEN = 3, // 监听中,接受客户端连接(listen socket使用)
            CONNECTING = 4, // 正在连接(connect失败的状态,之后会重新连接)
            CONNECTED = 5, // 已连接,可以收发数据
            HALFCLOSE = 6,
            PACCEPT = 7, // 等待连接(listen socket使用)
            BIND = 8,
        };

        explicit Socket(int fd):sock_fd_(fd) {}
        explicit Socket():sock_fd_(0),type_(Type::INVALID) {}
        int Init(int id, int fd, SocketProtocol protocol, uintptr_t opaque);
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
        void SetType(Type t) { type_ = t; }
        Type GetType() const { return type_; }
        int GetId() const { return id_; }
        void SetId(int id) { id_ = id; }

        bool SendBufferEmpty() { return high.empty() && low.empty(); }

        bool NoMoreSendingData() { return SendBufferEmpty() && dw_buffer_ && (sending_ & 0xffff) == 0; }

        bool CanDirectWrite(int id) { return id_ == id && NoMoreSendingData() && type_ == Socket::Type::CONNECTED && udp_connecting == 0;}

        uintptr_t GetOpaque() const { return opaque_; }
        void SetOpaque(uintptr_t opaque) { opaque_ = opaque; }

        SocketProtocol GetProtocol() const {return protocol_;}
        void SetProtocol(SocketProtocol protocol) { protocol_ = protocol;}

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
        std::atomic<Type> type_; // 状态
        uintptr_t opaque_{}; // socket关联的服务地址
        SocketProtocol protocol_{}; // 协议
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
