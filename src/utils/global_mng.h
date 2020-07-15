//
// ��ȡ����
//

#ifndef TINK_GLOBAL_MNG_H
#define TINK_GLOBAL_MNG_H

#include <memory>
#include <iserver.h>
#include <singleton.h>
#include <string>
#define READ_BUF_SIZE 1024
namespace tink {
    // �洢��ܵ�ȫ�ֲ���
    class GlobalMng {
    public:
        GlobalMng();
        // ���ó�ʼ��
        int Init();
        int Reload();
        std::shared_ptr<IServer> GetServer() const;

        std::shared_ptr<std::string> GetHost() const;

        const std::shared_ptr<std::string> GetName() const;

        int getPort() const;

        std::shared_ptr<std::string> GetVersion() const;

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
        // �������������������
        uint32_t max_worker_task_len_;
    };

}



#endif //TINK_GLOBAL_MNG_H
