//
// Created by Ёблн on 2020/9/28.
//

#ifndef TINK_EPOLLER_H
#define TINK_EPOLLER_H

#include "poller.h"

namespace tink {
    class EPoller : public Poller {
    public:
        EPoller();

        bool Invalid() override;

        ~EPoller() override;

        int Add(int sock, void *ud) override;

        void Del(int sock) override;

        void Write(int sock, void *ud, bool enable) override;

        int Wait(EventList &e) override;
    };

}



#endif //TINK_EPOLLER_H
