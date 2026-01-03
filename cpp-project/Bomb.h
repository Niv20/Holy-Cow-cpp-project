#pragma once
#include "Point.h"
#include <vector>

// Forward declaration!
class Game;

class Bomb {
    Point position;
    int roomIdx;
    int ticksLeft; // game cycles until explosion
public:
    Bomb(Point pos, int room, int delay = 5)
        : position(pos), roomIdx(room), ticksLeft(delay) {}

    // Decrease timer, return true if should explode now
    bool tick() {
        if (ticksLeft > 0) --ticksLeft;
        return ticksLeft <= 0;
    }

    Point getPosition() const { return position; }
    int getRoomIdx() const { return roomIdx; }

    void setPosition(Point p) { position = p; }
    
    // Explode: destroy weak walls, obstacles, damage players
    // Returns indices of players who were hit
    std::vector<int> explode(Game& game);
    
    // Static methods for managing all bombs
    static void tickAndHandleAll(std::vector<Bomb>& bombs, Game& game);
    static void place(std::vector<Bomb>& bombs, int roomIdx, const Point& pos, int delay = 5);
};
