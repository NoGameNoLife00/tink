//
// ��ȡ����
//

#ifndef TINK_GLOBAL_MNG_H
#define TINK_GLOBAL_MNG_H

#include <memory>
#include <iserver.h>
#include <singleton.h>
#include <string>
#include <easylogging++.h>

#define READ_BUF_SIZE 1024

#define GlobalInstance (tink::Singleton<tink::GlobalMng>::GetInstance())
namespace tink {
    // �洢��ܵ�ȫ�ֲ���
    class GlobalMng {
    public:
        GlobalMng();
        // ���ó�ʼ��
        int Init();
        int Reload();
        const std::shared_ptr<IServer>& GetServer() const;

        void SetServer(std::shared_ptr<IServer>&& s);

        const StringPtr& GetHost() const;

        const StringPtr& GetName() const;

        int GetPort() const;

        const StringPtr& GetVersion() const;

        int GetMaxConn() const;

        uint32_t GetMaxPackageSize() const;
        uint32_t GetWorkerPoolSize() const;
    private:
        // ���
        std::shared_ptr<std::string> version_;
        int max_conn_;
        uint32_t max_package_size_;

        // ȫ��Server����
        std::shared_ptr<IServer> server_;
        std::shared_ptr<std::string> host_;
        std::shared_ptr<std::string> name_;
        int port_;
        uint32_t worker_pool_size_;
        // ��������������������
        uint32_t max_worker_task_len_;
    };
    extern el::Logger* logger;
}



#endif //TINK_GLOBAL_MNG_H