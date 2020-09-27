#include <config_mng.h>
#include <conn_manager.h>

namespace tink {
    void ConnManager::Add(ConnectionPtr &&conn) {
        {
            std::lock_guard<Mutex> guard(mutex_);
            conn_map_.insert(std::pair<uint32_t, ConnectionPtr>(conn->GetConnId(), conn));
        }
        spdlog::info("conn add to mgr successfully: conn_id ={}, size={}", conn->GetConnId(), Size());
    }

    void ConnManager::Remove(ConnectionPtr &&conn) {
        {
            std::lock_guard<Mutex> guard(mutex_);
            conn_map_.erase(conn->GetConnId());
        }
        spdlog::info("conn remove from mgr successfully: conn_id ={}, size={}", conn->GetConnId(), Size());
    }

    ConnectionPtr ConnManager::Get(const uint32_t conn_id) {
        std::lock_guard<Mutex> guard(mutex_);
        auto it = conn_map_.find(conn_id);
        if (it != conn_map_.end()){
            return it->second;
        }
        return ConnectionPtr();
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


