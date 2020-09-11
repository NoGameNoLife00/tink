#pragma once

#include <cstdint>
#include <map>
#include <grid.h>
#include <player.h>

namespace logic {
    class AOI {
    public:
        uint32_t min_x; // ��߽�����
        uint32_t min_y; // �ϱ߽�����
        uint32_t max_x; // �ұ߽�����
        uint32_t max_y; // �±߽�����
        uint32_t cnt_x; // x���������
        uint32_t cnt_y; // y���������
        std::map<int, GridPtr> grid_map;

        typedef std::vector<GridPtr> GridList;

        AOI(uint32_t min_x, uint32_t min_y, uint32_t max_x,
                uint32_t max_y, uint32_t cnt_x, uint32_t cnt_y);
        uint32_t GetWidth() const; // ��ø��ӿ�
        uint32_t GetLength() const ; // ��ø��ӳ�
        std::string ToString() const ;
        uint32_t GetGidByPos(float x, float y) const ;
        // ���ݸ���Gid�õ���Χ�Ź��񼯺�
        void GetSurroundGridsByGid(uint32_t gid, GridList& vec) const ;
        // ���������������Χ�Ź�������id�б�
        void GetPidListByPos(float x, float y, PidList& player_ids) const ;
        // ���һ��pid��������
        void AddPidToGrid(int32_t pid, uint32_t gid);
        // �Ƴ�һ�����ӵ�pid
        void RemovePidFromGrid(int32_t pid, uint32_t gid);
        // ͨ��Gid��ȡȫ����pid
        void GetPidListByGid(uint32_t gid, PidList& player_ids);
        // ͨ�����꽫player��ӵ�һ��������
        void AddToGridByPos(int32_t pid, float x, float y);
        // ͨ�������һ����ҴӸ�����ɾ��
        void RemoveFromGridByPos(int32_t pid, float x, float y);
    };
};

