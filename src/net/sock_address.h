#ifndef TINK_SOCK_ADDRESS_H
#define TINK_SOCK_ADDRESS_H

#include <copyable.h>
#include <cstdint>
#include <string_util.h>
#include <memory>
#include <netinet/in.h>
#include <socket_api.h>
namespace tink {
    enum class SocketProtocol {
        TCP = 0,
        UDP = 1,
        UDPv6 = 2,
        UNKNOWN = 255,
    };

    class SockAddress : public copyable {
    public:
        explicit SockAddress(uint16_t port = 0, bool loopback_only = false, bool ipv6 = false );
        SockAddress(std::string_view ip, uint16_t port, bool ipv6 = false);
        explicit SockAddress(const struct sockaddr_in& addr) : addr_(addr) {}
        explicit SockAddress(const struct sockaddr_in6& addr) : addr6_(addr) {}
        int Init(const void *addr, uint16_t port, bool ipv6 = false);
        sa_family_t Family() const { return addr_.sin_family; }
        string ToIp() const ;
        string ToIpPort() const ;
        uint16_t ToPort() const ;
        const struct sockaddr* GetSockAddr() const { return SocketApi::SockAddrCast(&addr6_); }
        void SetSockAddrInet6(const struct sockaddr_in6& addr6) { addr6_ = addr6; }
        uint32_t IpNetEndian() const;
        uint16_t PortNetEndian() const { return addr_.sin_port; }
        static bool Resolve(std::string_view hostname, SockAddress* result);
        void SetScopeId(uint32_t scope_id);
        int GenUpdAddress(SocketProtocol protocol, uint8_t *udp_address);
    private:
        union {
            struct sockaddr_in addr_;
            struct sockaddr_in6 addr6_;
        };
    };

    typedef std::shared_ptr<SockAddress> SockAddressPtr;
}


#endif //TINK_SOCK_ADDRESS_H
