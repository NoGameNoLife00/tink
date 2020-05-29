#ifndef TINK_BASE_ROUTER_H
#define TINK_BASE_ROUTER_H


#include <irouter.h>
namespace tink {
    //
    // 所有的Router继承BaseRouter类，因为某些Router类不需要某种Handle
    //
    class BaseRouter : public IRouter {
    public:
        int PreHandle(IRequest &request);

        int Handle(IRequest &request);

        int PostHandle(IRequest &request);
    };
}



#endif //TINK_BASE_ROUTER_H
