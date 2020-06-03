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
        const std::shared_ptr<IServer> &GetServer() const;

        const std::shared_ptr<std::string> &GetHost() const;

        const std::shared_ptr<std::string> &GetName() const;

        int getPort() const;

        const std::shared_ptr<std::string> &GetVersion() const;

        int GetMaxConn() const;

        uint GetMaxPackageSize() const;

    private:
        // ���
        std::shared_ptr<std::string> version_;
        int max_conn_;
        uint max_package_size_;

        // ȫ��Server����
        std::shared_ptr<IServer> server_;
        std::shared_ptr<std::string> host_;
        std::shared_ptr<std::string> name_;
        int port_;
    };

}



#endif //TINK_GLOBAL_MNG_H
