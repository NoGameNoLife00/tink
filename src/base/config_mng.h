//
// ��ȡ����
//

#ifndef TINK_CONFIG_MNG_H
#define TINK_CONFIG_MNG_H

#include <memory>
#include <server.h>
#include <singleton.h>
#include <string>
#include <spdlog/spdlog.h>

#define READ_BUF_SIZE 1024

//#define ConfigMngInstance (tink::Singleton<tink::ConfigMng>::GetInstance())

namespace tink {
    // �洢��ܵ�ȫ�ֲ���
    class Config {
    public:
        // ���ó�ʼ��
        int Init();
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
        // �������������������
        uint32_t max_worker_task_len_;
        string daemon_;
        string module_path_;
        bool profile_;
        int harbor_;
    };

    typedef std::shared_ptr<Config> ConfigPtr;
}



#endif //TINK_CONFIG_MNG_H
