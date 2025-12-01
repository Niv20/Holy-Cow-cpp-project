#pragma once
#include "Point.h"
#include <vector>

struct ObCell { int roomIdx; Point pos; };

// Represents an obstacle made of contiguous '*' cells (4-neighborhood), possibly across rooms.
class Obstacle {
    std::vector<ObCell> cells; // absolute positions with room index
public:
    Obstacle(const std::vector<ObCell>& pts) : cells(pts) {}
    int size() const { return (int)cells.size(); }
    const std::vector<ObCell>& getCells() const { return cells; }

    bool contains(int roomIdx, const Point& p) const {
        for (auto& c : cells) if (c.roomIdx==roomIdx && c.pos.x==p.x && c.pos.y==p.y) return true; return false;
    }

    // Attempt push by dx,dy with given speed (number of steps). Requires force >= size.
    bool canPush(int dx, int dy, int force, class Game& game, int speed = 1) const;
    void applyPush(int dx, int dy, class Game& game, int speed = 1);
};
