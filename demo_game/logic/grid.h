#pragma once

#include <set>
#include <common.h>
#include <vector>
#include <shared_mutex>

namespace logic {
    struct Grid {
        uint32_t gid;
        uint32_t min_x;
        uint32_t min_y;
        uint32_t max_x;
        uint32_t max_y;
        std::set<int> player_set;
        Grid(int gid, int min_x, int min_y, int max_x, int max_y);
        // 格子添加玩家id
        void Add(int player_id);
        // 格子删除玩家id
        void Remove(int player_id);
        // 获取格子中所有玩家id
        void GetPlayerIds(std::vector<int>& ids);

        std::string ToString();

        friend std::ostream& operator<<(std::ostream& os, Grid& grid);

    private:
        mutable std::shared_mutex mutex_;
    };
    typedef std::shared_ptr<Grid> GridPtr;
}



