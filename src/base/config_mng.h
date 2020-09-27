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

#define ConfigMngInstance (tink::Singleton<ConfigMng>::GetInstance())

namespace tink {
    // �洢��ܵ�ȫ�ֲ���
    class ConfigMng {
    public:
        // ���ó�ʼ��
        int Init();
        int Reload();
        const std::shared_ptr<Server>& GetServer() const;

        void SetServer(std::shared_ptr<Server>&& s);

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
        int max_conn_;
        uint32_t max_package_size_;

        // ȫ��Server����
        std::shared_ptr<Server> server_;
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
}



#endif //TINK_CONFIG_MNG_H
