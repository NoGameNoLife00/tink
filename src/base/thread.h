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
        explicit Thread(ThreadFunc func, const std::string& name = "");

        void Start();
        void Join();
        bool Started() const {return started_;};
        const std::string& Name() const {return name_;};
        static uint32_t NumCrated() {return num_created_.load();};
        static void * StartThread(void* obj);
    private:
        void Create_();
        static std::atomic_uint num_created_;
        ThreadFunc func_;
        bool       started_;
        bool       joined_;
        pthread_t      pid_;
        std::string     name_;

        struct ThreadData {
            ThreadFunc thread_func;
            std::string name;
        };
    };
}




#endif //TINK_THREAD_H
