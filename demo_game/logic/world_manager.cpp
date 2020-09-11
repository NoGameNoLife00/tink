#include <world_manager.h>

#define AOI_MIN_X 0
#define AOI_MIN_Y 0
#define AOI_MAX_X 0
#define AOI_MAX_Y 0
#define AOI_CNT_X 0
#define AOI_CNY_Y 0

namespace logic {
//    std::shared_ptr<logic::WorldManager> WorldMngObj = tink::Singleton<logic::WorldManager>::GetInstance();

    void WorldManager::Init() {
        aoi = std::make_shared<AOI>(AOI_MIN_X, AOI_MIN_Y, AOI_MAX_X, AOI_MAX_Y, AOI_CNT_X, AOI_CNY_Y);
    }

    void WorldManager::AddPlayer(PlayerPtr player) {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_);
        if (player_map.find(player->pid) != player_map.end()) {
            return;
        }
        player_map.insert({player->pid, player});
        aoi->AddToGridByPos(player->pid, player->x, player->y);
    }

    void WorldManager::RemovePlayer(int32_t pid) {
        std::unique_lock<std::shared_timed_mutex> lock(mutex_);
        player_map.erase(pid);
    }

    PlayerPtr WorldManager::GetPlayerByPid(int32_t pid) {
        std::shared_lock<std::shared_timed_mutex> lock(mutex_);
        auto it = player_map.find(pid);
        if (it != player_map.end()) {
             return it->second;
        }
        return PlayerPtr();
    }

    void WorldManager::GetAllPlayers(PlayerList &result) {
        std::shared_lock<std::shared_timed_mutex> lock(mutex_);
        for (auto&& p : player_map) {
            result.emplace_back(p.second);
        }
    }

}
