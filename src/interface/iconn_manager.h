//
// 连接管理模块抽象类
//

#ifndef TINK_ICONN_MANAGER_H
#define TINK_ICONN_MANAGER_H

#include "iconnection.h"

namespace tink {
    class IConnManager {
    public:
        // 添加连接
        virtual void Add(IConnectionPtr &conn) = 0;
        // 删除连接
        virtual void Remove(IConnectionPtr &conn) = 0;
        // 根据connID获取连接
        virtual int32_t Get(const uint32_t conn_id, IConnectionPtr &conn) = 0;
        // 得到当前连接数
        virtual uint32_t Size() = 0;
        // 清除终止所以连接
        virtual void ClearConn() = 0;
    };
}

#endif //TINK_ICONN_MANAGER_H
