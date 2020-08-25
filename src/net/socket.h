#ifndef TINK_SOCKET_H
#define TINK_SOCKET_H

#include <cstddef>
#include <cstdint>
#include <endian.h>
#include <netinet/tcp.h>
#include <noncopyadble.h>
#include <netinet/in.h>
#include <string_util.h>
#include <socket_api.h>

namespace tink {
    class SockAddress {
        explicit SockAddress(uint16_t port = 0, bool loopback_only = false, bool ipv6 = false );
        SockAddress(StringArg ip, uint16_t port, bool ipv6 = false);
        explicit SockAddress(const struct sockaddr_in& addr) : addr_(addr) {}
        explicit SockAddress(const struct sockaddr_in6& addr) : addr6_(addr) {}
        sa_family_t Family() const { return addr_.sin_family; }
        string ToIp() const ;
        string ToIpPort() const ;
        uint16_t ToPort() const ;
        const struct sockaddr* GetSockAddr() const { return SocketApi::SockAddrCast(&addr6_); }
        void SetSockAddrInet6(const struct sockaddr_in6& addr6) { addr6_ = addr6; }
        uint32_t IpNetEndian() const;
        uint16_t PortNetEndian() const { return addr_.sin_port; }
        static bool Resolve(StringArg hostname, SockAddress* result);
        void SetScopeId(uint32_t scope_id);
    public:
        union {
            struct sockaddr_in addr_;
            struct sockaddr_in6 addr6_;
        };
    };

    class Socket : noncopyable {
    public:
        explicit Socket(int fd):sock_fd_(fd) {}
        ~Socket();
        int GetSockFd() const { return sock_fd_; }

        int GetTcpInfo(struct tcp_info* info) const ;
        int GetTcpInfoString(char *buf, int len) const ;
        void BindAddress(const SockAddress& addr);
        void Listen();

        int Accept(SockAddress* peer_addr);
        void ShutDownWrite();
        void SetTcpNoDelay(bool active);
        void SetKeepAlive(bool active);
    private:
        const int sock_fd_;
    };
}


#endif //TINK_SOCKET_H
