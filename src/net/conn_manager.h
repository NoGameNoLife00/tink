//
// 连接管理模块
//

#ifndef TINK_CONN_MANAGER_H
#define TINK_CONN_MANAGER_H

#include <common.h>
#include <connection.h>
#include <unordered_map>
#include <mutex>
#include <server.h>

namespace tink {
    class Connection;
    typedef std::shared_ptr<Connection> ConnectionPtr;

    class ConnManager {
    public:
        // 添加连接
        void Add(ConnectionPtr &&conn);
        // 删除连接
        void Remove(ConnectionPtr &&conn);
        // 根据connID获取连接
        ConnectionPtr Get(uint32_t conn_id) ;
        // 得到当前连接数
        uint32_t Size();
        // 清除终止所以连接
        void ClearConn();

    private:
        typedef std::unordered_map<uint32_t, ConnectionPtr> ConnectionMap;
        ConnectionMap conn_map_;
        Mutex mutex_;
    };
}




#endif //TINK_CONN_MANAGER_H
