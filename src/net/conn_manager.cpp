
#include <global_mng.h>
#include <error_code.h>
#include "conn_manager.h"

namespace tink {
    void ConnManager::Add(IConnectionPtr &&conn) {
        {
            std::lock_guard<Mutex> guard(mutex_);
            conn_map_.insert(std::pair<uint32_t, IConnectionPtr>(conn->GetConnId(), conn));
        }
        spdlog::info("conn add to mgr successfully: conn_id ={}, size={}", conn->GetConnId(), Size());
    }

    void ConnManager::Remove(IConnectionPtr &&conn) {
        {
            std::lock_guard<Mutex> guard(mutex_);
            conn_map_.erase(conn->GetConnId());
        }
        spdlog::info("conn remove from mgr successfully: conn_id ={}, size={}", conn->GetConnId(), Size());
    }

    IConnectionPtr ConnManager::Get(const uint32_t conn_id) {
        std::lock_guard<Mutex> guard(mutex_);
        auto it = conn_map_.find(conn_id);
        if (it != conn_map_.end()){
            return it->second;
        }
        return IConnectionPtr();
    }

    uint32_t ConnManager::Size() {
        return conn_map_.size();
    }

    void ConnManager::ClearConn() {
        std::lock_guard<Mutex> guard(mutex_);
        for (auto && conn : conn_map_) {
            conn.second->Stop();
        }
        conn_map_.clear();
    }
}


