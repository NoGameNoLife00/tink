//
// Created by Ёблн on 2020/8/20.
//

#ifndef TINK_THREAD_H
#define TINK_THREAD_H

#include <functional>
#include <atomic>

namespace tink{
    typedef pid_t tid_t;
    class Thread {
    public:
        typedef std::function<void()> ThreadFunc;
        void Start();
        void Join();
        bool Started() const {return started_;};
        const std::string& Name() const {return name_;};
        static uint32_t NumCrated() {return num_created_.load();};
    private:
        static std::atomic_uint num_created_;
        ThreadFunc func_;
        bool       started_;
        bool       joined_;
        pthread_t      pid_;
        std::string     name_;

    };
}




#endif //TINK_THREAD_H
