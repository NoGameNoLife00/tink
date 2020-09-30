#include <unistd.h>
#include <sys/socket.h>
#include <config_mng.h>
#include <assert.h>
#include <arpa/inet.h>
#include <socket_api.h>

namespace tink {
    int SocketApi::Connect(int fd, const struct sockaddr *addr) {
        return ::connect(fd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
    }

    int SocketApi::Read(int fd, void *buf, size_t count) {
        return ::read(fd, buf, count);
    }

    int SocketApi::Write(int fd, const void *buf, size_t count) {
        return ::write(fd, buf, count);
    }

    int SocketApi::SendTo(int fd, const void *buf, size_t count, int flag,
                          const struct sockaddr *addr, socklen_t addr_len) {

        return ::sendto(fd, buf, count, flag, addr, addr_len);
    }

    void SocketApi::Close(int fd) {
        if (::close(fd) < 0) {
            spdlog::warn("socket.close");
        }
    }

    void SocketApi::ShutdownWrite(int fd) {
        if (::shutdown(fd, SHUT_WR) < 0)
        {
            spdlog::warn("SocketApi::shutdownWrite");
        }
    }

    int SocketApi::GetSocketError(int fd) {
        int optval;
        socklen_t optlen = static_cast<socklen_t>(sizeof optval);

        if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
        {
            return errno;
        }
        else
        {
            return optval;
        }
    }

    void SocketApi::ToIpPort(char *buf, size_t size, const struct sockaddr *addr) {
        ToIp(buf,size, addr);
        size_t end = ::strlen(buf);
        const struct sockaddr_in* addr4 = SockAddrInCast(addr);
        uint16_t port = NetworkToHost16(addr4->sin_port);
        assert(size > end);
        snprintf(buf+end, size-end, ":%u", port);
    }

    void SocketApi::ToIp(char *buf, size_t size, const struct sockaddr *addr) {
        if (addr->sa_family == AF_INET)
        {
            assert(size >= INET_ADDRSTRLEN);
            const struct sockaddr_in* addr4 = SockAddrInCast(addr);
            ::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
        }
        else if (addr->sa_family == AF_INET6)
        {
            assert(size >= INET6_ADDRSTRLEN);
            const struct sockaddr_in6* addr6 = SockAddrIn6Cast(addr);
            ::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
        }
    }

    void SocketApi::FromIpPort(const char *ip, uint16_t port, struct sockaddr_in *addr) {
        addr->sin_family = AF_INET;
        addr->sin_port = HostToNetwork16(port);
        if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0)
        {
            spdlog::error("SocketApi.fromIpPort");
        }
    }

    void SocketApi::FromIpPort(const char *ip, uint16_t port, struct sockaddr_in6 *addr) {
        addr->sin6_family = AF_INET6;
        addr->sin6_port = HostToNetwork16(port);
        if (::inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0)
        {
            spdlog::error("SocketApi.fromIpPort");
        }
    }

    const struct sockaddr *SocketApi::SockAddrCast(const struct sockaddr_in *addr) {
        return static_cast<const struct sockaddr*>(static_cast<const void*>(addr));
    }

    const struct sockaddr *SocketApi::SockAddrCast(const struct sockaddr_in6 *addr) {
        return static_cast<const struct sockaddr*>(static_cast<const void*>(addr));
    }

    struct sockaddr *SocketApi::SockAddrCast(struct sockaddr_in6 *addr) {
        return static_cast<struct sockaddr*>(static_cast<void*>(addr));;
    }

    const struct sockaddr_in *SocketApi::SockAddrInCast(const struct sockaddr *addr) {
        return static_cast<const struct sockaddr_in*>(static_cast<const void*>(addr));
    }

    const struct sockaddr_in6 *SocketApi::SockAddrIn6Cast(const struct sockaddr *addr) {
        return static_cast<const struct sockaddr_in6*>(static_cast<const void*>(addr));
    }

    struct sockaddr_in6 SocketApi::GetLocalAddr(int fd) {
        struct sockaddr_in6 local_addr;
        memset(&local_addr, 0, sizeof local_addr);
        socklen_t addrlen = static_cast<socklen_t>(sizeof local_addr);
        if (::getsockname(fd, SockAddrCast(&local_addr), &addrlen) < 0)
        {
            spdlog::error("SocketApi.getLocalAddr");
        }
        return local_addr;
    }

    struct sockaddr_in6 SocketApi::GetPeerAddr(int fd) {
        struct sockaddr_in6 peer_addr;
        memset(&peer_addr, 0, sizeof peer_addr);
        socklen_t addrlen = static_cast<socklen_t>(sizeof peer_addr);
        if (::getpeername(fd, SockAddrCast(&peer_addr), &addrlen) < 0)
        {
            spdlog::error("SocketApi::getPeerAddr");
        }
        return peer_addr;
    }

    bool SocketApi::IsSelfConnect(int fd) {
        struct sockaddr_in6 local_addr = GetLocalAddr(fd);
        struct sockaddr_in6 peer_addr = GetPeerAddr(fd);
        if (local_addr.sin6_family == AF_INET)
        {
            const struct sockaddr_in* laddr4 = reinterpret_cast<struct sockaddr_in*>(&local_addr);
            const struct sockaddr_in* raddr4 = reinterpret_cast<struct sockaddr_in*>(&peer_addr);
            return laddr4->sin_port == raddr4->sin_port
                   && laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
        }
        else if (local_addr.sin6_family == AF_INET6)
        {
            return local_addr.sin6_port == peer_addr.sin6_port
                   && memcmp(&local_addr.sin6_addr, &peer_addr.sin6_addr, sizeof local_addr.sin6_addr) == 0;
        }
        else
        {
            return false;
        }
    }

    int SocketApi::Bind(int fd, const struct sockaddr *addr) {
        if (int ret; (ret = ::bind(fd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)))) < 0) {
            spdlog::critical("socket.bind {}:{}", strerror(errno), errno);
            return ret;
        }
    }

    int SocketApi::Listen(int fd, int backlog) {
        if (int ret; (ret = ::listen(fd, backlog)) < 0) {
            spdlog::critical("socket.listen {}:{}", strerror(errno), errno);
            return ret;
        }
    }

    int SocketApi::Accept(int fd, struct sockaddr_in6 *addr) {
        socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);
        int conn_fd = ::accept4(fd, SockAddrCast(addr),
                                &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (conn_fd < 0) {
            int savedErrno = errno;
            spdlog::error("socket.accept");
            switch (savedErrno)
            {
                case EAGAIN:
                case ECONNABORTED:
                case EINTR:
                case EPROTO: // ???
                case EPERM:
                case EMFILE: // per-process lmit of open file desctiptor ???
                    // expected errors
                    errno = savedErrno;
                    break;
                case EBADF:
                case EFAULT:
                case EINVAL:
                case ENFILE:
                case ENOBUFS:
                case ENOMEM:
                case ENOTSOCK:
                case EOPNOTSUPP:
                    // unexpected errors
                    spdlog::critical("unexpected error of ::accept {}", savedErrno);
                    break;
                default:
                    spdlog::critical("unknown error of ::accept {}", savedErrno);
                    break;
            }
        }
        return conn_fd;
    }

    int SocketApi::Create(sa_family_t family, bool nonblock, bool udp) {
        int sock_fd = 0;
        int sock_type = udp ? SOCK_DGRAM : SOCK_STREAM;
        if (nonblock) {
            sock_fd = ::socket(family, sock_type | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
            if (sock_fd < 0)
            {
                spdlog::critical("sockets.Create {}:{}",strerror(errno), errno);
            }
        } else {
            sock_fd = ::socket(family, sock_type, 0);
            if (sock_fd < 0)
            {
                spdlog::critical("sockets.create {}:{}", strerror(errno), errno);
            }
        }
        return sock_fd;
    }


}