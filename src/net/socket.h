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
#include <copyable.h>
#include <sock_address.h>

namespace tink {
    class Socket : noncopyable {
    public:
        explicit Socket(int fd):sock_fd_(fd) {}
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
    private:
        const int sock_fd_;
    };
}


#endif //TINK_SOCKET_H
