#include <sock_address.h>
#include <cstring>
#include <assert.h>
#include <netdb.h>
#include <config_mng.h>
#include "socket.h"

namespace tink {
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

    SockAddress::SockAddress(std::string_view ip, uint16_t port, bool ipv6) {
        if (ipv6)
        {
            memset(&addr6_, 0, sizeof addr6_);
            SocketApi::FromIpPort(ip.data(), port, &addr6_);
        }
        else
        {
            memset(&addr_, 0, sizeof addr_);
            SocketApi::FromIpPort(ip.data(), port, &addr_);
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

    bool SockAddress::Resolve(std::string_view hostname, SockAddress *result) {
        assert(result != NULL);
        struct hostent hent;
        struct hostent* he = NULL;
        int err_no = 0;
        memset(&hent, 0, sizeof(hent));

        int ret = gethostbyname_r(hostname.data(), &hent, t_resolveBuffer, sizeof t_resolveBuffer, &he, &err_no);
        if (ret == 0 && he != nullptr) {
            assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
            result->addr_.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
            return true;
        } else {
            if (ret) {
                spdlog::error("SockAddress.resolve");
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

    int SockAddress::Init(const void *addr, uint16_t port, bool ipv6) {
        if (ipv6) {
            memset(&addr6_, 0, sizeof addr6_);
            addr6_.sin6_family = AF_INET6;
            addr6_.sin6_port = SocketApi::HostToNetwork16(port);
            memcpy(&addr6_.sin6_addr, addr, sizeof(addr_.sin_addr));
            return sizeof(addr6_);
        } else {
            memset(&addr_, 0, sizeof addr_);
            addr_.sin_family = AF_INET;
            addr_.sin_port = SocketApi::HostToNetwork16(port);
            memcpy(&addr_.sin_addr, addr, sizeof(addr_.sin_addr));
            return sizeof(addr_);
        }
    }

    int SockAddress::GenUpdAddress(int protocol, uint8_t *udp_address) {
        int addrsz = 1;
        udp_address[0] = (uint8_t)protocol;
        if (protocol == PROTOCOL_UDP) {
            memcpy(udp_address+addrsz, &addr_.sin_port, sizeof(addr_.sin_port));
            addrsz += sizeof(addr_.sin_port);
            memcpy(udp_address+addrsz, &addr_.sin_addr, sizeof(addr_.sin_addr));
            addrsz += sizeof(addr_.sin_addr);
        } else {
            memcpy(udp_address+addrsz, &addr6_.sin6_port, sizeof(addr6_.sin6_port));
            addrsz += sizeof(addr6_.sin6_port);
            memcpy(udp_address+addrsz, &addr6_.sin6_addr, sizeof(addr6_.sin6_addr));
            addrsz += sizeof(addr6_.sin6_addr);
        }
        return addrsz;
    }

}