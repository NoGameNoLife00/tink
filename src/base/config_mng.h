//
// ∂¡»°≈‰÷√
//

#ifndef TINK_CONFIG_MNG_H
#define TINK_CONFIG_MNG_H

#include <memory>
#include <singleton.h>
#include <string>
#include <spdlog/spdlog.h>
#include <string_util.h>

#define READ_BUF_SIZE 1024

//#define ConfigMngInstance (tink::Singleton<tink::ConfigMng>::GetInstance())

namespace tink {
    // ¥Ê¥¢≈‰÷√
    using std::string;
    class Config {
    public:
        // ≈‰÷√≥ı ºªØ
        int Init(std::string_view path);
        int Reload();
        const string& GetHost() const;
        const string& GetName() const;
        int GetPort() const;
        const string& GetVersion() const;
        int GetMaxConn() const;
        const string& GetDaemon() const { return daemon_; }
        const string& GetModulePath() const { return module_path_; }
        bool GetProfile() const { return profile_; }
        int GetHarbor() const { return harbor_; }
        uint32_t GetMaxPackageSize() const;
        uint32_t GetWorkerPoolSize() const;
        const string& GetBootstrap() { return bootstrap_; }
    private:
        void Default_();
        void InitLog_();
        int max_conn_;
        uint32_t max_package_size_;
        string version_;
        string host_;
        string name_;
        string log_name_;
        int port_;
        uint32_t worker_pool_size_;
        uint32_t max_worker_task_len_;
        string daemon_;
        string module_path_;
        string bootstrap_;
        bool profile_;
        int harbor_;
    };

    typedef std::shared_ptr<Config> ConfigPtr;
}



#endif //TINK_CONFIG_MNG_H
