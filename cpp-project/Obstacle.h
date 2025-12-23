#pragma once
#include "Point.h"
#include <vector>

class ObCell { 
private:
    int roomIdx_; 
    Point pos_; 
    
public:
    ObCell(int room, Point p) : roomIdx_(room), pos_(p) {}
    
    int getRoomIdx() const { return roomIdx_; }
    Point getPos() const { return pos_; }
    
    void setRoomIdx(int room) { roomIdx_ = room; }
    void setPos(Point p) { pos_ = p; }
};

// Represents an obstacle made of contiguous '*' cells (4-neighborhood), possibly across rooms.
class Obstacle {
    std::vector<ObCell> cells; // absolute positions with room index
public:
    Obstacle(const std::vector<ObCell>& pts) : cells(pts) {}
    int size() const { return (int)cells.size(); }
    const std::vector<ObCell>& getCells() const { return cells; }

    bool contains(int roomIdx, const Point& p) const {
        for (const auto& c : cells) {
            if (c.getRoomIdx() == roomIdx && c.getPos().x == p.x && c.getPos().y == p.y) 
                return true;
        }
        return false;
    }

    // Attempt push by dx,dy with given speed (number of steps). Requires force >= size.
    bool canPush(int dx, int dy, int force, class Game& game, int speed = 1) const;
    void applyPush(int dx, int dy, class Game& game, int speed = 1);
    
    // Static method to scan all obstacles in the world
    static void scanAllObstacles(std::vector<class Screen>& world, const class RoomConnections& roomConnections);
    
    // Static: Find obstacle at position in a screen
    static Obstacle* findAt(class Screen& screen, int roomIdx, const Point& p);
};