//
// Created by admin on 2020/5/29.
//

#ifndef TINK_SINGLETON_H
#define TINK_SINGLETON_H

#include <memory>
#include <mutex>
#include "noncopyable.h"
#include <cassert>
namespace tink {
    template<typename T>
    struct has_no_destroy
    {
        template <typename C> static char test(decltype(&C::no_destroy));
        template <typename C> static int32_t test(...);
        const static bool value = sizeof(test<T>(0)) == 1;
    };

    template<typename T>
    class Singleton : noncopyable
    {
    public:
        Singleton() = delete;
        ~Singleton() = delete;

        static T& GetInstance()
        {
            pthread_once(&ponce_, &Singleton::New);
            assert(value_ != NULL);
            return *value_;
        }

    private:
        static void New()
        {
            value_ = new T();
            if (!has_no_destroy<T>::value)
            {
                ::atexit(Delete);
            }
        }

        static void Delete()
        {
            typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
            T_must_be_complete_type dummy; (void) dummy;

            delete value_;
            value_ = NULL;
        }

    private:
        static pthread_once_t ponce_;
        static T*             value_;
    };

    template<typename T>
    pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

    template<typename T>
    T* Singleton<T>::value_ = NULL;
}

#endif //TINK_SINGLETON_H
