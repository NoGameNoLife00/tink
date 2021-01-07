#ifndef TINK_POLLER_H
#define TINK_POLLER_H

#include "common.h"

namespace tink {
    constexpr int MAX_EVENT = 64;
    typedef int PollFd;
    struct Event {
        void * s;
        bool read;
        bool write;
        bool error;
        bool eof;
    };
    typedef std::vector<Event> EventList;
    class Poller {
    public:
        static Poller* NewDefaultPoller();
        virtual bool Invalid() { return false; }
        virtual void Release() {}
        virtual int Add(int sock, void *ud) {return 0;}
        virtual void Del(int sock) {}
        virtual void Write(int sock, void *ud, bool enable) {}
        virtual int Wait(EventList& e) { return 0; }
        virtual ~Poller() {};
    protected:
        PollFd poll_fd_;
    };

    typedef std::shared_ptr<Poller> PollerPtr;
}

#endif //TINK_POLLER_H
