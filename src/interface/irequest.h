//
// �ͻ��������������Ϣ
//

#ifndef TINK_IREQUEST_H
#define TINK_IREQUEST_H

#include <iconnection.h>

class IRequest {
    // ��ȡ��ǰ����
    virtual IConnection* GetConnection() = 0;
    // ��ȡ�������Ϣ����
    virtual char* GetData();
};

#endif //TINK_IREQUEST_H
