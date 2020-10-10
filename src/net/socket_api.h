#ifndef TINK_SOCKET_API_H
#define TINK_SOCKET_API_H

#include <cstdint>
#include <endian.h>
#include <cstddef>
#include <bits/sockaddr.h>
namespace tink {
    namespace SocketApi {
        int Create(sa_family_t family, bool nonblock = false, bool udp = false);
        int Connect(int fd, const struct ::sockaddr* addr);
        int Bind(int fd, const struct sockaddr* addr);
        int Listen(int fd, int backlog = SOMAXCONN);
        int Accept(int fd, struct sockaddr_in6* addr);
        int Read(int fd, void *buf, size_t count);
        int Write(int fd, const void *buf, size_t count);
        int SendTo(int fd, const void *buf, size_t count, int flag, const struct sockaddr* addr, socklen_t addr_len);
        void Close(int fd);
        void ShutdownWrite(int fd);
        int GetSocketError(int fd);
        void ToIpPort(char* buf, size_t size, const struct sockaddr* addr);
        void ToIp(char* buf, size_t size, const struct ::sockaddr* addr);
        void FromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr);
        void FromIpPort(const char* ip, uint16_t port, struct sockaddr_in6* addr);
        const struct sockaddr* SockAddrCast(const struct sockaddr_in* addr);
        const struct sockaddr* SockAddrCast(const struct sockaddr_in6* addr);
        struct sockaddr* SockAddrCast(struct sockaddr_in6* addr);
        const struct sockaddr_in* SockAddrInCast(const struct sockaddr* addr);
        const struct sockaddr_in6* SockAddrIn6Cast(const struct sockaddr* addr);
        struct sockaddr_in6 GetLocalAddr(int fd);
        struct sockaddr_in6 GetPeerAddr(int fd);
        bool IsSelfConnect(int fd);
        void NonBlocking(int fd);

        void SetKeepAlive(int fd, bool active);

        inline uint64_t HostToNetwork64(uint64_t host64)
        {
            return htobe64(host64);
        }

        inline uint32_t HostToNetwork32(uint32_t host32)
        {
            return htobe32(host32);
        }

        inline uint16_t HostToNetwork16(uint16_t host16)
        {
            return htobe16(host16);
        }

        inline uint64_t NetworkToHost64(uint64_t net64)
        {
            return be64toh(net64);
        }

        inline uint32_t NetworkToHost32(uint32_t net32)
        {
            return be32toh(net32);
        }

        inline uint16_t NetworkToHost16(uint16_t net16)
        {
            return be16toh(net16);
        }


    }
}

#endif //TINK_SOCKET_API_H
