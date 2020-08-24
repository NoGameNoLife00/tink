//
// Created by admin on 2020/5/29.
//

#ifndef TINK_SINGLETON_H
#define TINK_SINGLETON_H

#include <memory>
#include <mutex>

namespace tink {
    // ��ģ��
    template<typename T>
    class Singleton {
    public:
        template<typename ...Args>
        static std::shared_ptr<T> GetInstance(Args&&... args) {
            if (!instance_) {
                std::lock_guard<Mutex> gLock(mutex_);
                if (nullptr == instance_) {
                    instance_ = std::make_shared<T>(std::forward<Args>(args)...);
                }
            }
            return instance_;
        }

        //����������������һ�㲻��Ҫ����������������������
        static void DesInstance() {
            if (instance_) {
                instance_.reset();
                instance_ = nullptr;
            }
        }

    private:
        static std::shared_ptr<T> instance_;
        static Mutex mutex_;

        explicit Singleton();
        Singleton(const Singleton&) = delete;
        Singleton& operator=(const Singleton&) = delete;
        ~Singleton();
    };

    template<typename T>
    std::shared_ptr<T> Singleton<T>::instance_ = nullptr;

    template<typename T>
    Mutex Singleton<T>::mutex_;
}

#endif //TINK_SINGLETON_H
