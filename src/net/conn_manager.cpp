
#include <global_mng.h>
#include <error_code.h>
#include "conn_manager.h"

namespace tink {
    void ConnManager::Add(tink::IConnectionPtr &conn) {
        {
            std::lock_guard<std::mutex> guard(mutex_);
            conn_map_.insert(std::pair<uint32_t, IConnectionPtr>(conn->GetConnId(), conn));
        }
        logger->info("conn add to mgr successfully: conn_id =%v, size=%v", conn->GetConnId(), Size());
    }

    void ConnManager::Remove(tink::IConnectionPtr &conn) {
        {
            std::lock_guard<std::mutex> guard(mutex_);
            conn_map_.erase(conn->GetConnId());
        }
        logger->info("conn remove from mgr successfully: conn_id =%v, size=%v", conn->GetConnId(), Size());
    }

    int32_t ConnManager::Get(const uint32_t conn_id, IConnectionPtr &conn) {
        std::lock_guard<std::mutex> guard(mutex_);
        auto it = conn_map_.find(conn_id);
        if (it != conn_map_.end()){
            conn = it->second;
            return E_OK;
        }
        return E_CONN_NOT_FIND;
    }

    uint32_t ConnManager::Size() {
        return conn_map_.size();
    }

    void ConnManager::ClearConn() {
        std::lock_guard<std::mutex> guard(mutex_);
        for (auto && conn : conn_map_) {
            conn.second->Stop();
        }
        conn_map_.clear();
    }
}


