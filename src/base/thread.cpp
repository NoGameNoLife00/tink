//
// Created by Ёблн on 2020/8/20.
//

#include <assert.h>
#include <memory>
#include <execinfo.h>
#include <cxxabi.h>
#include <thread.h>
#include <config_mng.h>
#include <sys/prctl.h>
#include "thread.h"

namespace tink {
    namespace CurrentThread {
        thread_local const char* t_thread_name = "None";

        std::string StackTrace(bool demangle) {
            std::string stack;
            const int max_frames = 200;
            void* frame[max_frames];
            int nptrs = backtrace(frame, max_frames);
            char** strings = backtrace_symbols(frame, nptrs);
            if (strings)
            {
                size_t len = 256;
                char* demangled = demangle ? static_cast<char*>(malloc(len)) : nullptr;
                for (int i = 1; i < nptrs; ++i)  // skipping the 0-th, which is this function
                {
                    if (demangle)
                    {
                        char* left_par = nullptr;
                        char* plus = nullptr;
                        for (char* p = strings[i]; *p; ++p)
                        {
                            if (*p == '(')
                                left_par = p;
                            else if (*p == '+')
                                plus = p;
                        }

                        if (left_par && plus)
                        {
                            *plus = '\0';
                            int status = 0;
                            char* ret = abi::__cxa_demangle(left_par+1, demangled, &len, &status);
                            *plus = '+';
                            if (status == 0)
                            {
                                demangled = ret;  // ret could be realloc()
                                stack.append(strings[i], left_par+1);
                                stack.append(demangled);
                                stack.append(plus);
                                stack.push_back('\n');
                                continue;
                            }
                        }
                    }
                    // Fallback to mangled names
                    stack.append(strings[i]);
                    stack.push_back('\n');
                }
                free(demangled);
                free(strings);
            }
            return stack;
        }
    }

    std::atomic_uint Thread::num_created_;
    Thread::Thread(tink::Thread::ThreadFunc func, const std::string &name)
        : started_(false), joined_(false),
        pid_(0), func_(std::move(func)),
        name_(name), latch_(1)
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
        ThreadData *data = new ThreadData {func_, name_, &latch_};
        if (pthread_create(&pid_, NULL, &StartThread, data)) {
            started_ = false;
            delete data;
            spdlog::error("pthread_create failed {}:", errno, strerror(errno));
        } else {
            latch_.Wait();
        }
    }

    void * Thread::StartThread(void *obj) {
        ThreadData* data = static_cast<ThreadData*>(obj);
        data->RunThread();
        delete data;
        return nullptr;
    }

    int Thread::Join() {
        assert(started_);
        assert(!joined_);
        joined_ = true;
        return pthread_join(pid_, NULL);
    }

    Thread::~Thread() {
        if (started_ && !joined_) {
            pthread_detach(pid_);
        }
    }

    void Thread::ThreadData::RunThread() {
        latch->CountDown();
        latch = nullptr;
        CurrentThread::t_thread_name = name.empty() ? "tinkThread" : name.c_str();
        prctl(PR_SET_NAME, CurrentThread::t_thread_name);
        try {
            thread_func();
            CurrentThread::t_thread_name = "finished";
        } catch (const std::exception& ex) {
            CurrentThread::t_thread_name = "crashed";
            fprintf(stderr, "exception caught in Thread %s\n", name.c_str());
            fprintf(stderr, "reason: %s\n", ex.what());
            abort();
        } catch (...) {
            CurrentThread::t_thread_name = "crashed";
            fprintf(stderr, "unknown exception caught in Thread %s\n", name.c_str());
            throw; // rethrow
        }
    }
}
