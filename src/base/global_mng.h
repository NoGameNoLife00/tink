//
// 读取配置
//

#ifndef TINK_GLOBAL_MNG_H
#define TINK_GLOBAL_MNG_H

#include <memory>
#include <server.h>
#include <singleton.h>
#include <string>
#include <spdlog/spdlog.h>

#define READ_BUF_SIZE 1024

#define GlobalInstance (tink::Singleton<tink::GlobalMng>::GetInstance())

namespace tink {
    // 存储框架的全局参数
    class GlobalMng {
    public:
        GlobalMng();
        // 配置初始化
        int Init();
        int Reload();
        const std::shared_ptr<Server>& GetServer() const;

        void SetServer(std::shared_ptr<Server>&& s);

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
        std::shared_ptr<Server> server_;
        string version_;
        string host_;
        string name_;
        string log_name_;
        int port_;
        uint32_t worker_pool_size_;
        // 框架允许的最大任务数量
        uint32_t max_worker_task_len_;
    };
}



#endif //TINK_GLOBAL_MNG_H
