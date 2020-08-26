#include <sys/socket.h>
#include <global_mng.h>
#include <unistd.h>
#include <netinet/in.h>
#include <assert.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <socket.h>

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
        SocketApi::Bind(sock_fd_, addr.GetSockAddr());
    }

    void Socket::Listen() {
        SocketApi::Listen(sock_fd_);
    }

    int Socket::Accept(SockAddress &peer_addr) {
        struct sockaddr_in6 addr;
        memset(&addr, 0, sizeof addr);
        int conn_fd = SocketApi::Accept(sock_fd_, &addr);
        if (conn_fd >= 0) {
            peer_addr.SetSockAddrInet6(addr);
        }
        return conn_fd;
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

}

