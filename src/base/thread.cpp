//
// Created by Ёблн on 2020/8/20.
//

#include <assert.h>
#include <memory>
#include "thread.h"
namespace tink {
    Thread::Thread(tink::Thread::ThreadFunc func, const std::string &name)
        : started_(false), joined_(false), pid_(0), func_(std::move(func)), name_(name)
    {
        Create_();
    }

    void Thread::Create_() {
        int t_num = num_created_.fetch_add(1);
        if (name_.empty()) {
            name_ = "Thread" + t_num;
        }
    }

    void Thread::Start() {
        assert(!started_);
        started_ = true;
        ThreadData *data = new ThreadData {func_, name_};
        if (pthread_create(&pid_, NULL, &StartThread, data)) {
            started_ = false;
            delete data;
        }
    }

    void * Thread::StartThread(void *obj) {
        ThreadData* data = static_cast<ThreadData*>(obj);
    }

}
