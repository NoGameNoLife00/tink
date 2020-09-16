#ifndef TINK_BASE_ROUTER_H
#define TINK_BASE_ROUTER_H
#include <request.h>
namespace tink {
    //
    // 所有的Router继承BaseRouter类，因为某些Router类不需要某种Handle
    //
    class Request;

    class BaseRouter {
    public:
        // 处理conn业务之前的hook
        virtual int PreHandle(Request &request);
        // 处理conn业务的主hook
        virtual int Handle(Request &request);
        // 处理conn业务之后的hook
        virtual int PostHandle(Request &request);
    };

}



#endif //TINK_BASE_ROUTER_H
