#ifndef TINK_BASE_ROUTER_H
#define TINK_BASE_ROUTER_H


#include <irouter.h>
namespace tink {
    //
    // ���е�Router�̳�BaseRouter�࣬��ΪĳЩRouter�಻��Ҫĳ��Handle
    //
    class BaseRouter : public IRouter {
    public:
        int PreHandle(IRequest &request);

        int Handle(IRequest &request);

        int PostHandle(IRequest &request);
    };
}



#endif //TINK_BASE_ROUTER_H
