#include <epoller.h>
#include <poll.h>
#include "poller.h"


using namespace tink;

Poller *Poller::NewDefaultPoller() {
    return new EPoller();
}
