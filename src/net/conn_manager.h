//
// 连接管理模块
//

#ifndef TINK_CONN_MANAGER_H
#define TINK_CONN_MANAGER_H

#include <type.h>
#include <iconnection.h>
#include <unordered_map>
#include <mutex>
#include <iconn_manager.h>

namespace tink {
    class ConnManager : public IConnManager{
    public:
        void Add(IConnectionPtr &&conn) override;

        void Remove(IConnectionPtr &&conn) override;

        IConnectionPtr Get(const uint32_t conn_id) override;

        uint32_t Size() override;

        void ClearConn() override;

    private:
        typedef std::unordered_map<uint32_t, IConnectionPtr> ConnectionMap;
        ConnectionMap conn_map_;
        std::mutex mutex_;
    };
}




#endif //TINK_CONN_MANAGER_H
