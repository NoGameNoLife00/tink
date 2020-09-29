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
namespace tink {

    typedef struct WriteBuffer_ {
        const void * buffer;
        char *ptr;
        size_t sz;
        bool userobj;
        uint8_t upd_address[UDP_ADDRESS_SIZE];
    }WriteBuffer;

    typedef struct SocketStat_ {
        uint64_t rtime;
        uint64_t wtime;
        uint64_t read;
        uint64_t write;
    }SocketStat;

    typedef std::list<WriteBuffer> WbList;

    class Socket : noncopyable {
    public:
//        explicit Socket(int fd):sock_fd_(fd) {}
        int Init(int id, int fd, int protocol, uintptr_t opaque);
        void Destroy();
        ~Socket();
        int GetSockFd() const { return sock_fd_; }
        int GetTcpInfo(struct tcp_info* info) const ;
        int GetTcpInfoString(char *buf, int len) const ;
        void BindAddress(const SockAddress& addr);
        void Listen();
        int Accept(SockAddress &peer_addr);
        void ShutDownWrite();
        void SetTcpNoDelay(bool active);
        void SetKeepAlive(bool active);

        socklen_t UdpAddress(const uint8_t udp_address[UDP_ADDRESS_SIZE], SockAddress& sa);

        void SetType(int t) { type_ = t; }
        int GetType() const { return type_; }

        int GetId() const { return id_; }
        void SetId(int id) { id_ = id; }

        bool SendBufferEmpty() {
            return high_.empty() && low_.empty();
        }

        bool NoMoreSendingData() {
            return SendBufferEmpty() && dw_buffer_ && (sending_ & 0xffff) == 0;
        }

        bool CanDirectWrite(int id) {
            return id_ == id && NoMoreSendingData() && type_ == SOCKET_TYPE_CONNECTED && udp_connecting_ == 0;
        }

        uintptr_t GetOpaque() { return opaque_; }
        void SetOpaque(uintptr_t opaque) { opaque_ = opaque; }

        uint8_t GetProtocol() const {return protocol_;}
        void SetProtocol(uint8_t protocol) { protocol_ = protocol;}

        WbList& GetHigh() { return high_; }
        WbList& GetLow() { return low_; }

        FixBufferPtr& GetDWBuffer() { return dw_buffer_; }

        void SetDwBuffer(FixBufferPtr& buf) {
            dw_buffer_ = std::move(buf);
        }

        void CloneDwBuffer(DataPtr buff, int sz, int offset) {
            dw_buffer_.reset(std::make_shared<FixBuffer>());

        }

        mutable std::recursive_mutex mutex;

        const uint8_t * GetUdpAddress() { return p_.udp_address; }

        void StatWrite(int n, uint64_t time) {
            stat_.write += n;
            stat_.wtime = time;
        }
    private:
        int id_;
        int sock_fd_;
        int type_;
        uintptr_t opaque_;
        uint8_t protocol_;
        uint64_t wb_size_;
        WbList high_;
        WbList low_;
        std::atomic_uint32_t sending_;
        SocketStat stat_;
        uint16_t udp_connecting_;
        int64_t warn_size_;
        union {
            int size;
            uint8_t udp_address[UDP_ADDRESS_SIZE];
        } p_;
        FixBufferPtr dw_buffer_;
//        int dw_offset_;
//        const void * dw_buffer_;
//        size_t dw_size_;


    };
    typedef std::shared_ptr<Socket> SocketPtr;
}


#endif //TINK_SOCKET_H
