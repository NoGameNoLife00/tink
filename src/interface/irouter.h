//
// ·�ɳ���ӿ�
//

#ifndef TINK_IROUTER_H
#define TINK_IROUTER_H

#include "irequest.h"
namespace  tink{
    class IRouter {
    public:
        // ����connҵ��֮ǰ��hook
        virtual int PreHandle(IRequest &request) = 0;
        // ����connҵ�����hook
        virtual int Handle(IRequest &request) = 0;
        // ����connҵ��֮���hook
        virtual int PostHandle(IRequest &request) = 0;
    };
    typedef std::shared_ptr<IRouter> IRouterPtr;
}


#endif //TINK_IROUTER_H
