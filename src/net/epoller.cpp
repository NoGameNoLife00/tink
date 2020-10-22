#include <epoller.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <error_code.h>

namespace tink {
    bool EPoller::Invalid() {
        return poll_fd_ == -1;
    }

    EPoller::~EPoller() {
        close(poll_fd_);
    }

    int EPoller::Add(int sock, void *ud) {
        struct epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.ptr = ud;
        if (epoll_ctl(poll_fd_, EPOLL_CTL_ADD, sock, &ev) == -1) {
            return E_FAILED;
        }
        return E_OK;
    }

    void EPoller::Del(int sock) {
        epoll_ctl(poll_fd_, EPOLL_CTL_DEL, sock , nullptr);
    }

    void EPoller::Write(int sock, void *ud, bool enable) {
        struct epoll_event ev{};
        ev.events = EPOLLIN | (enable ? EPOLLOUT : 0);
        ev.data.ptr = ud;
        epoll_ctl(poll_fd_, EPOLL_CTL_MOD, sock, &ev);
    }

    int EPoller::Wait(tink::EventList &e) {
        struct epoll_event ev[MAX_EVENT];
        int n = epoll_wait(poll_fd_ , ev, MAX_EVENT, -1);
        int i;
        for (i=0;i<n;i++) {
            e[i].s = ev[i].data.ptr;
            unsigned flag = ev[i].events;
            e[i].write = (flag & EPOLLOUT) != 0;
            e[i].read = (flag & (EPOLLIN | EPOLLHUP)) != 0;
            e[i].error = (flag & EPOLLERR) != 0;
            e[i].eof = false;
        }
        return n;
    }

    EPoller::EPoller() {
        poll_fd_ = epoll_create1(EPOLL_CLOEXEC);
        if (poll_fd_ < 0) {
            fprintf(stderr, "create epoll failed");
        }
    }



}

