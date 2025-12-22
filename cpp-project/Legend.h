#pragma once
#include <vector>
#include "Screen.h"
#include "Point.h"

class Legend {
    std::vector<Point> roomLegendPos; // indexed by room idx
public:
    Legend() = default;
    void ensureRooms(size_t count);
    void locateLegendForRoom(int roomIdx, const Screen& s);
    void drawAnchor(int roomIdx) const;
    void drawLegend(int roomIdx, int lives, int points, char p1Inv, char p2Inv);
    static void scanAllLegends(std::vector<Screen>& world, Legend& legend);
};