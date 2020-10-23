#include <sys/socket.h>
#include <config.h>
#include <socket.h>

namespace tink {


    Socket::~Socket() {

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
        SocketApi::SetKeepAlive(sock_fd_, active);
    }

    int Socket::Init(int id, int fd, int protocol, uintptr_t opaque) {
        id_ = id;
        sock_fd_ = fd;
        sending_ = ID_TAG16(id) << 16 | 0;
        protocol_ = protocol;
        p_.size = MIN_READ_BUFFER;
        opaque_ = opaque;
        wb_size_ = 0;
        warn_size_ = 0;
        assert(high.empty());
        assert(low.empty());
        dw_buffer_ = nullptr;
        memset(&stat_, 0, sizeof(stat_));
        return 0;
    }

    void Socket::Destroy() {
    }

    socklen_t Socket::UdpAddress(const uint8_t udp_address[UDP_ADDRESS_SIZE], SockAddress &sa) {
        int type = (uint8_t)udp_address[0];
        if (type != protocol_)
            return 0;
        uint16_t port = 0;
        memcpy(&port, udp_address+1, sizeof(uint16_t));
        switch (protocol_) {
            case PROTOCOL_UDP:
                return sa.Init(udp_address + 1 + sizeof(uint16_t), port, false);
            case PROTOCOL_UDPv6:
                return sa.Init(udp_address + 1 + sizeof(uint16_t), port, true);
        }
        return 0;
    }

    void Socket::IncSendingRef(int id) {
        if (protocol_ != PROTOCOL_TCP)
            return;
        for (;;) {
            if ((sending_ >> 16) == ID_TAG16(id)) {
                if ((sending_ & 0xffff) == 0xffff) {
                    // s->sending may overflow (rarely), so busy waiting here for socket thread dec it. see issue #794
                    continue;
                }
                // inc sending only matching the same socket id
                auto cur = sending_.load();
                if (sending_.compare_exchange_strong(cur, sending_ + 1)) {
                    return;
                }
                // atom inc failed, retry
            } else {
                // socket id changed, just return
                return;
            }
        }
    }

    void Socket::DecSendingRef(int id) {
        if (id == id_ && protocol_ == PROTOCOL_TCP) {
            assert((sending_ & 0xffff) != 0);
            sending_ -= 1;
        }
    }

    void Socket::SetReuseAddr(bool active) {
        int optval = active ? 1 : 0;
        ::setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEADDR,
                     &optval, static_cast<socklen_t>(sizeof optval));
    }

    void Socket::SetReusePort(bool active) {
        int optval = active ? 1 : 0;
        int ret = ::setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEPORT,
                               &optval, static_cast<socklen_t>(sizeof optval));
        if (ret < 0 && active)
        {
            fprintf(stderr, "SO_REUSEPORT failed.");
        }
    }

    void Socket::Close() {
        SocketApi::Close(sock_fd_);
    }

    bool Socket::Reserve(int id) {
        uint8_t pl = SOCKET_TYPE_INVALID;
        if (type_.compare_exchange_strong(pl, SOCKET_TYPE_RESERVE)) {
            id_ = id;
            protocol_ = PROTOCOL_UNKNOWN;
            udp_connecting = 0;
            sock_fd_ = -1;
            return true;
        }
        return false;
    }

    void Socket::RaiseUnComplete() {
        high.emplace_back(low.front());
        low.pop_front();
    }

}

