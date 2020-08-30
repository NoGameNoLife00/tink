#include <sstream>
#include "aoi.h"
namespace logic {

    AOI::AOI(uint32_t min_x, uint32_t min_y, uint32_t max_x,
              uint32_t max_y, uint32_t cnt_x, uint32_t cnt_y)
            : min_x(max_x), min_y(min_y), max_x(max_x),
            max_y(max_y), cnt_x(cnt_x), cnt_y(cnt_y) {
        uint32_t width = GetWidth();
        uint32_t length = GetLength();
        int gid = 0;
        for (int y = 0; y < cnt_y; y++) {
            for (int x = 0; x < cnt_x; x++) {
                gid = y * cnt_x + x;
                std::shared_ptr<Grid> grid = std::make_shared<Grid>(gid,
                        min_x + x * width, min_y + y * length,
                        min_x + (x+1) * width, min_y + (y+1) * length);
                grid_map.insert(std::pair<int, std::shared_ptr<Grid>>(gid, grid));
            }
        }
    }

    uint32_t AOI::GetWidth() const {
        return (max_x - min_x) / cnt_x;
    }

    uint32_t AOI::GetLength() const {
        return (max_y - min_y) / cnt_y;
    }

    std::string AOI::ToString() const {
        std::stringstream ss;
        ss << "AOI maxX:" << max_x << " maxY:" << max_y
            << " minX:" << min_x << " minY:" << min_y
            << " cntX:" << cnt_x << " cntY:" << cnt_y << "\n";
        for (auto it = grid_map.begin(); it != grid_map.end(); it++) {
            ss << *(it->second) << "\n";
        }
        return ss.str();
    }

    void AOI::GetSurroundGridsByGid(const uint32_t gid, GridList &vec) const {
        auto it = grid_map.find(gid);
        if (it == grid_map.end()) {
            return;
        }
        vec.emplace_back(it->second);
        int idx = gid % cnt_x;
        if (idx > 0) {
            auto it = grid_map.find(gid-1);
            if (it != grid_map.end()) {
                vec.emplace_back(it->second);
            }
        }

        if (idx < (cnt_x - 1)) {
            auto it = grid_map.find(gid+1);
            if (it != grid_map.end()) {
                vec.emplace_back(it->second);
            }
        }
        std::vector<int> grids_x;

        for (auto& g : vec) {
            grids_x.emplace_back(g->gid);
        }

        for (auto& id : grids_x) {
            uint32_t idy = id / cnt_y;
            if (idy > 0) {
                auto grid_it = grid_map.find(id - cnt_x);
                if (grid_it != grid_map.end()) {
                    vec.emplace_back(grid_it->second);
                }
            }
            if (idy < (cnt_y - 1)) {
                auto grid_it = grid_map.find(id + cnt_x);
                if (grid_it != grid_map.end()) {
                    vec.emplace_back(grid_it->second);
                }
            }
        }
    }

    uint32_t AOI::GetGidByPos(float x, float y) const {
        uint32_t idx = (x - min_x) / GetWidth();
        uint32_t idy = (y - min_y) / GetLength();
        return idy * cnt_x + idx;
    }


    void AOI::GetPidListByPos(float x, float y, PidList &player_ids) const {
        // 计算当前玩家的Gid
        uint32_t gid = GetGidByPos(x, y);
        std::vector<GridPtr> grids;
        GetSurroundGridsByGid(gid, grids);
        std::vector<int> pid_list;
        for (auto& g : grids) {
            g->GetPlayerIds(pid_list);
            player_ids.insert(player_ids.end(), pid_list.begin(), pid_list.end());
            pid_list.clear();
        }
    }

    void AOI::AddPidToGrid(int32_t pid, uint32_t gid) {
        auto it = grid_map.find(gid);
        if (it != grid_map.end()) {
            return;
        }
        it->second->Add(pid);
    }

    void AOI::RemovePidFromGrid(int32_t pid, uint32_t gid) {
        auto it = grid_map.find(gid);
        if (it != grid_map.end()) {
            return;
        }
        it->second->Remove(pid);
    }

    void AOI::GetPidListByGid(uint32_t gid, PidList& player_ids) {
        auto it = grid_map.find(gid);
        if (it != grid_map.end()) {
            return;
        }
        it->second->GetPlayerIds(player_ids);
    }

    void AOI::AddToGridByPos(int32_t pid, float x, float y) {
        uint32_t gid = GetGidByPos(x, y);
        AddPidToGrid(pid, gid);
    }

    void AOI::RemoveFromGridByPos(int32_t pid, float x, float y) {
        uint32_t gid = GetGidByPos(x, y);
        RemovePidFromGrid(pid, gid);
    }

}