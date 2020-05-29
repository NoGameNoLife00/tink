//
// 客户端请求的链接信息
//

#ifndef TINK_IREQUEST_H
#define TINK_IREQUEST_H

#include <iconnection.h>
#include <type.h>
namespace tink {
    class IRequest {
    public:
        // 获取当前连接
        virtual IConnection & GetConnection() = 0;
        // 获取请求的消息数据
        virtual std::shared_ptr<byte> GetData() = 0;
    };
}


#endif //TINK_IREQUEST_H
