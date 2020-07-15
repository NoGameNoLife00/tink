//
// 路由抽象接口
//

#ifndef TINK_IROUTER_H
#define TINK_IROUTER_H

#include "irequest.h"
namespace  tink{
    class IRouter {
    public:
        // 处理conn业务之前的hook
        virtual int PreHandle(IRequest &request) = 0;
        // 处理conn业务的主hook
        virtual int Handle(IRequest &request) = 0;
        // 处理conn业务之后的hook
        virtual int PostHandle(IRequest &request) = 0;
    };
    typedef std::shared_ptr<IRouter> IRouterPtr;
}


#endif //TINK_IROUTER_H
