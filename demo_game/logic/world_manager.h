#pragma once

#include "aoi.h"
#include "player.h"
#include <shared_mutex>
#include <memory>
#include <map>
#include <singleton.h>


#define WorldMngInstance (tink::Singleton<logic::WorldManager>::GetInstance())
namespace logic {
    typedef std::vector<PlayerPtr> PlayerList;
    class WorldManager {
    public:
        std::shared_ptr<AOI> aoi;
        std::map<int32_t, PlayerPtr> player_map;
        void Init();
        void AddPlayer(PlayerPtr player);
        void RemovePlayer(int32_t pid);
        PlayerPtr GetPlayerByPid(int32_t pid);
        void GetAllPlayers(PlayerList& result);
    private:
        std::shared_timed_mutex mutex_;
    };
//    extern std::shared_ptr<WorldManager> WorldMngObj;
}





