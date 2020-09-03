//
// 读取配置
//

#ifndef TINK_GLOBAL_MNG_H
#define TINK_GLOBAL_MNG_H

#include <memory>
#include <iserver.h>
#include <singleton.h>
#include <string>
#include <spdlog/spdlog.h>

#define READ_BUF_SIZE 1024

#define GlobalInstance (tink::Singleton<tink::GlobalMng>::GetInstance())
//extern el::Logger* logger;

namespace tink {
    // 存储框架的全局参数
    class GlobalMng {
    public:
        GlobalMng();
        // 配置初始化
        int Init();
        int Reload();
        const std::shared_ptr<IServer>& GetServer() const;

        void SetServer(std::shared_ptr<IServer>&& s);

        const string& GetHost() const;

        const string& GetName() const;

        int GetPort() const;

        const string& GetVersion() const;

        int GetMaxConn() const;

        uint32_t GetMaxPackageSize() const;
        uint32_t GetWorkerPoolSize() const;
    private:
        int max_conn_;
        uint32_t max_package_size_;

        // 全局Server对象
        std::shared_ptr<IServer> server_;
        string version_;
        string host_;
        string name_;
        int port_;
        uint32_t worker_pool_size_;
        // 框架允许的最大任务数量
        uint32_t max_worker_task_len_;
    };


//    template <typename T, typename... Args>
//    void inline spdlog::info(const char* s, const T& value, const Args&... args) {
//        logger->info(s, value, args...);
//    }
//    template <typename T>
//    inline void spdlog::info(const T& value) {
//        logger->info(value);
//    }
//
//    template <typename T, typename... Args>
//    void inline spdlog::warn(const char* s, const T& value, const Args&... args) {
//        logger->warn(s, value, args...);
//    }
//    template <typename T>
//    inline void spdlog::warn(const T& value) {
//        logger->warn(value);
//    }
//
//
//    template <typename T, typename... Args>
//    void inline spdlog::error(const char* s, const T& value, const Args&... args) {
//        logger->error(s, value, args...);
//    }
//    template <typename T>
//    inline void spdlog::error(const T& value) {
//        logger->error(value);
//    }
//
//    template <typename T, typename... Args>
//    void inline spdlog::critical(const char* s, const T& value, const Args&... args) {
//        logger->fatal(s, value, args...);
//    }
//    template <typename T>
//    inline void spdlog::critical(const T& value) {
//        logger->fatal(value);
//    }
//
//    template <typename T, typename... Args>
//    void inline spdlog::debug(const char* s, const T& value, const Args&... args) {
//        logger->debug(s, value, args...);
//    }
//    template <typename T>
//    inline void spdlog::debug(const T& value) {
//        logger->debug(value);
//    }

}



#endif //TINK_GLOBAL_MNG_H
