//
// Created by Ёблн on 2020/8/20.
//

#ifndef TINK_THREAD_H
#define TINK_THREAD_H

#include <functional>
#include <atomic>
#include <count_down_latch.h>

namespace tink{

    namespace CurrentThread {
        extern thread_local const char * t_thread_name;
        inline const char* name() {
            return t_thread_name;
        }
        std::string StackTrace(bool demangle);
    }


    class Thread : noncopyable {
    public:
        typedef std::function<void()> ThreadFunc;
        explicit Thread(ThreadFunc func, std::string_view name = "");
        ~Thread();
        void Start();
        int Join();
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
        CountDownLatch latch_;
        struct ThreadData {
            ThreadFunc thread_func;
            std::string name;
            CountDownLatch* latch;
            void RunThread();
        };
    };
}




#endif //TINK_THREAD_H
