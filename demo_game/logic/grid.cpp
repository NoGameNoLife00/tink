#include <sstream>
#include <grid.h>

namespace logic {
    Grid::Grid(int gid, int min_x, int min_y, int max_x, int max_y) :
            gid(gid), min_x(min_x), min_y(min_y),
            max_x(max_x), max_y(max_y) {
    }

    void Grid::Add(int player_id) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        player_set.emplace(player_id);
    }

    void Grid::Remove(int player_id) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        player_set.erase(player_id);
    }

    void Grid::GetPlayerIds(std::vector<int> &ids) {
        std::shared_lock<std::shared_mutex> guard(mutex_);
        ids.assign(player_set.begin(), player_set.end());
    }

    std::string Grid::ToString() {
        std::shared_lock<std::shared_mutex> guard(mutex_);
        std::stringstream ss;
        ss << "Gid: " << gid << " minX: " << min_x << " minY: "
           << min_y << " maxX: " << max_x
           << " maxY " << max_y << " playerIds:[";
        for (auto it = player_set.begin(); it != player_set.end(); it++) {
            ss << *it << " ";
        }
        ss << "]";
        return ss.str();
    }

    std::ostream &operator<<(std::ostream &os, Grid& grid) {
        os << grid.ToString();
        return os;
    }

}


