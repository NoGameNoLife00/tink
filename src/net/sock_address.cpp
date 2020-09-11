#include <sock_address.h>
#include <cstring>
#include <assert.h>
#include <netdb.h>
#include <global_mng.h>

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
        int err_no = 0;
        memset(&hent, 0, sizeof(hent));

        int ret = gethostbyname_r(hostname.c_str(), &hent, t_resolveBuffer, sizeof t_resolveBuffer, &he, &err_no);
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

}