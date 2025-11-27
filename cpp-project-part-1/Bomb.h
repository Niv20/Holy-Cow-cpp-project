#pragma once
#include "Point.h"

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
};
