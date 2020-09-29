#ifndef TINK_POLLER_H
#define TINK_POLLER_H

#include <common.h>

namespace tink {
    typedef int PollFd;
    typedef struct Event_ {
        void * s;
        bool read;
        bool write;
        bool error;
        bool eof;
    } Event;
    typedef std::vector<Event> EventList;
    class Poller {
    public:
        static Poller* NewDefaultPoller();
        virtual bool Invalid();
        virtual void Release();
        virtual int Add(int sock, void *ud);
        virtual void Del(int sock);
        virtual void Write(int sock, void *ud, bool enable);
        virtual int Wait(EventList& e);
        virtual ~Poller();
    protected:
        PollFd poll_fd_;
    };

    typedef std::shared_ptr<Poller> PollerPtr;
}

#endif //TINK_POLLER_H
