#pragma once

#include <set>
#include <type.h>
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
        mutable std::shared_mutex mutex;
        Grid(int gid, int min_x, int min_y, int max_x, int max_y);
        // ����������id
        void Add(int player_id);
        // ����ɾ�����id
        void Remove(int player_id);
        // ��ȡ�������������id
        void GetPlayerIds(std::vector<int>& ids);

        std::string ToString();

        friend std::ostream& operator<<(std::ostream& os, Grid& grid);
    };
    typedef std::shared_ptr<Grid> GridPtr;
}



