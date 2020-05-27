//
// ·�ɳ���ӿ�
//

#ifndef TINK_IROUTER_H
#define TINK_IROUTER_H

#include "irequest.h"

class IRouter {
public:
    // ����connҵ��֮ǰ��hook
    virtual int PreHandle(IRequest &request) = 0;
    // ����connҵ�����hook
    virtual int Handle(IRequest &request) = 0;
    // ����connҵ��֮���hook
    virtual int PostHandle(IRequest &request) = 0;
};


#endif //TINK_IROUTER_H
