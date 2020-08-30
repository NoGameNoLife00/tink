#include <gtest/gtest.h>
#include "../demo_game/logic/aoi.h"

using namespace logic;

TEST(AOI_New, test1) {
    // Éú³Éaoi
    AOI aoi(0, 0, 250, 250, 5, 5);
    std::cout << aoi.ToString();
}

TEST(AOI_SURROUND, test2) {
    AOI aoi(0, 0, 250, 250, 5, 5);
    std::vector<GridPtr> vec(9);
    for (auto& grid : aoi.grid_map) {
        vec.clear();
        aoi.GetSurroundGridsByGid(grid.first, vec);
        printf("gid:%d, grids len=%d [ ", grid.first, vec.size());
        for(auto& cur_grid : vec) {
            printf("%d ", cur_grid->gid);
        }
        printf("]\n");
    }
}