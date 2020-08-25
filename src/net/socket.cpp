#include <sys/socket.h>
#include <global_mng.h>
#include <unistd.h>
#include <netinet/in.h>
#include <assert.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "socket.h"

namespace tink {


    Socket::~Socket() {
        SocketApi::Close(sock_fd_);
    }

    int Socket::GetTcpInfo(struct tcp_info *info) const {
        socklen_t len = sizeof(*info);
        memset(info, 0, len);
        return ::getsockopt(sock_fd_, SOL_TCP, TCP_INFO, info, &len);
    }

    int Socket::GetTcpInfoString(char *buf, int len) const {
        struct tcp_info info;
        int ret = GetTcpInfo(&info);
        if (ret == 0) {
            snprintf(buf, len, "unrecovered=%u "
                               "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
                               "lost=%u retrans=%u rtt=%u rttvar=%u "
                               "sshthresh=%u cwnd=%u total_retrans=%u",
                     info.tcpi_retransmits,  // Number of unrecovered [RTO] timeouts
                     info.tcpi_rto,          // Retransmit timeout in usec
                     info.tcpi_ato,          // Predicted tick of soft clock in usec
                     info.tcpi_snd_mss,
                     info.tcpi_rcv_mss,
                     info.tcpi_lost,         // Lost packets
                     info.tcpi_retrans,      // Retransmitted packets out
                     info.tcpi_rtt,          // Smoothed round trip time in usec
                     info.tcpi_rttvar,       // Medium deviation
                     info.tcpi_snd_ssthresh,
                     info.tcpi_snd_cwnd,
                     info.tcpi_total_retrans);  // Total retransmits for entire connection
        }
        return ret;
    }

    void Socket::BindAddress(const SockAddress &addr) {
//        SocketApi::Bind(sock_fd_, );
    }

    void Socket::Listen() {
        SocketApi::Listen(sock_fd_);
    }

    int Socket::Accept(SockAddress *peer_addr) {
        return 0;
    }

    void Socket::ShutDownWrite() {
        SocketApi::ShutdownWrite(sock_fd_);
    }

    void Socket::SetTcpNoDelay(bool active) {
        int optval = active ? 1 : 0;
        ::setsockopt(sock_fd_, IPPROTO_TCP, TCP_NODELAY,
                     &optval, static_cast<socklen_t>(sizeof optval));
    }

    void Socket::SetKeepAlive(bool active) {
        int optval = active ? 1 : 0;
        ::setsockopt(sock_fd_, SOL_SOCKET, SO_KEEPALIVE,
                     &optval, static_cast<socklen_t>(sizeof optval));
    }

    SockAddress::SockAddress(uint16_t port, bool loopback_only, bool ipv6) {
        static_assert(offsetof(SockAddress, addr6_) == 0, "addr6_ offset 0");
        static_assert(offsetof(SockAddress, addr_) == 0, "addr_ offset 0");
        if (ipv6)
        {
            memset(&addr6_, 0, sizeof addr6_);
            addr6_.sin6_family = AF_INET6;
            in6_addr ip = loopback_only ? in6addr_loopback : in6addr_any;
            addr6_.sin6_addr = ip;
            addr6_.sin6_port = SocketApi::HostToNetwork16(port);
        }
        else
        {
            memset(&addr_, 0, sizeof addr_);
            addr_.sin_family = AF_INET;
            in_addr_t ip = loopback_only ? INADDR_LOOPBACK : INADDR_ANY;
            addr_.sin_addr.s_addr = SocketApi::HostToNetwork32(ip);
            addr_.sin_port = SocketApi::HostToNetwork16(port);
        }

    }

    SockAddress::SockAddress(StringArg ip, uint16_t port, bool ipv6) {
        if (ipv6)
        {
            memset(&addr6_, 0, sizeof addr6_);
            SocketApi::FromIpPort(ip.c_str(), port, &addr6_);
        }
        else
        {
            memset(&addr_, 0, sizeof addr_);
            SocketApi::FromIpPort(ip.c_str(), port, &addr_);
        }
    }

    string SockAddress::ToIp() const {
        char buf[64] = "";
        SocketApi::ToIp(buf, sizeof buf, GetSockAddr());
        return buf;
    }

    string SockAddress::ToIpPort() const {
        char buf[64] = "";
        SocketApi::ToIpPort(buf, sizeof buf, GetSockAddr());
        return buf;
    }

    uint16_t SockAddress::ToPort() const {
        return SocketApi::NetworkToHost16(PortNetEndian());
    }

    uint32_t SockAddress::IpNetEndian() const {
        assert(Family() == AF_INET);
        return addr_.sin_addr.s_addr;
    }
    static thread_local char t_resolveBuffer[64 * 1024];

    bool SockAddress::Resolve(StringArg hostname, SockAddress *result) {
        assert(result != NULL);
        struct hostent hent;
        struct hostent* he = NULL;
        int herrno = 0;
        memset(&hent, 0, sizeof(hent));

        int ret = gethostbyname_r(hostname.c_str(), &hent, t_resolveBuffer, sizeof t_resolveBuffer, &he, &herrno);
        if (ret == 0 && he != NULL)
        {
            assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
            result->addr_.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
            return true;
        }
        else
        {
            if (ret)
            {
                logger->error("SockAddress.resolve");
            }
            return false;
        }
    }

    void SockAddress::SetScopeId(uint32_t scope_id) {
        if (Family() == AF_INET6)
        {
            addr6_.sin6_scope_id = scope_id;
        }
    }
}

