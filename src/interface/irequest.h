//
// �ͻ��������������Ϣ
//

#ifndef TINK_IREQUEST_H
#define TINK_IREQUEST_H

#include <iconnection.h>
#include <type.h>
namespace tink {
    class IRequest {
    public:
        // ��ȡ��ǰ����
        virtual IConnection & GetConnection() = 0;
        // ��ȡ�������Ϣ����
        virtual std::shared_ptr<byte> GetData() = 0;
    };
}


#endif //TINK_IREQUEST_H
