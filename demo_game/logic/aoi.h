#pragma once

#include <cstdint>
#include <map>
#include <grid.h>
#include <player.h>

namespace logic {
    class AOI {
    public:
        uint32_t min_x; // 左边界坐标
        uint32_t min_y; // 上边界坐标
        uint32_t max_x; // 右边界坐标
        uint32_t max_y; // 下边界坐标
        uint32_t cnt_x; // x方向格子数
        uint32_t cnt_y; // y方向格子数
        std::map<int, GridPtr> grid_map;

        typedef std::vector<GridPtr> GridList;

        AOI(uint32_t min_x, uint32_t min_y, uint32_t max_x,
                uint32_t max_y, uint32_t cnt_x, uint32_t cnt_y);
        uint32_t GetWidth() const; // 获得格子宽
        uint32_t GetLength() const ; // 获得格子长
        std::string ToString() const ;
        uint32_t GetGidByPos(float x, float y) const ;
        // 根据格子Gid得到周围九宫格集合
        void GetSurroundGridsByGid(uint32_t gid, GridList& vec) const ;
        // 根据玩家坐标获得周围九宫格的玩家id列表
        void GetPidListByPos(float x, float y, PidList& player_ids) const ;
        // 添加一个pid到格子中
        void AddPidToGrid(int32_t pid, uint32_t gid);
        // 移除一个格子的pid
        void RemovePidFromGrid(int32_t pid, uint32_t gid);
        // 通过Gid获取全部的pid
        void GetPidListByGid(uint32_t gid, PidList& player_ids);
        // 通过坐标将player添加到一个格子中
        void AddToGridByPos(int32_t pid, float x, float y);
        // 通过坐标把一个玩家从格子中删除
        void RemoveFromGridByPos(int32_t pid, float x, float y);
    };
};

