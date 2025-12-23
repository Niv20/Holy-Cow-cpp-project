#pragma once
#include "Point.h"
#include <vector>

// Forward declarations
class Screen;
class Game;
class Player;

// Represents a linear spring (# chars) adjacent to a wall at one end.
// Stores original cells so we can collapse visually and restore.
class Spring {
public:
    enum Orientation { Horizontal, Vertical };
    
private:
    int roomIdx_ = -1;
    Point anchorStart_; // first cell (closest to wall when compressed)
    int length_ = 0;
    Orientation orient_ = Horizontal;
    // Runtime compression state
    int compressedCount_ = 0; // how many chars currently compressed (player stands over them)
    bool releasing_ = false;
    int releaseTicksLeft_ = 0; // remaining boosted movement cycles
    int releaseSpeed_ = 0; // horizontal or vertical speed during release

public:
    Spring() = default;
    
    // Getters
    int getRoomIdx() const { return roomIdx_; }
    Point getAnchorStart() const { return anchorStart_; }
    int getLength() const { return length_; }
    Orientation getOrient() const { return orient_; }
    int getCompressedCount() const { return compressedCount_; }
    bool isReleasing() const { return releasing_; }
    int getReleaseTicksLeft() const { return releaseTicksLeft_; }
    int getReleaseSpeed() const { return releaseSpeed_; }
    
    // Setters
    void setRoomIdx(int idx) { roomIdx_ = idx; }
    void setAnchorStart(Point p) { anchorStart_ = p; }
    void setLength(int len) { length_ = len; }
    void setOrient(Orientation o) { orient_ = o; }
    void setCompressedCount(int count) { compressedCount_ = count; }
    void setReleasing(bool r) { releasing_ = r; }
    void setReleaseTicksLeft(int ticks) { releaseTicksLeft_ = ticks; }
    void setReleaseSpeed(int speed) { releaseSpeed_ = speed; }
};

class SpringData {
private:
    int roomIdx_;
    std::vector<Point> cells_; // all spring cells in order from wall
    int dirX_; // direction away from wall (toward free end)
    int dirY_;
    Point wallPos_; // adjacent wall position
    
public:
    SpringData(int room, const std::vector<Point>& springCells, int dx, int dy, Point wall)
        : roomIdx_(room), cells_(springCells), dirX_(dx), dirY_(dy), wallPos_(wall) {}
    
    // Getters
    int getRoomIdx() const { return roomIdx_; }
    const std::vector<Point>& getCells() const { return cells_; }
    int getDirX() const { return dirX_; }
    int getDirY() const { return dirY_; }
    Point getWallPos() const { return wallPos_; }
    
    int length() const { return (int)cells_.size(); }
    
    // Find which cell index player is on (returns -1 if not on this spring)
    int findCellIndex(const Point& p) const {
        for (int i = 0; i < (int)cells_.size(); ++i) {
            if (cells_[i].x == p.x && cells_[i].y == p.y) return i;
        }
        return -1;
    }
    
    // Static: Find spring at position in a screen
    static SpringData* findAt(Screen& screen, const Point& p);
};

// Spring logic helper class
class SpringLogic {
public:
    // Release spring and launch player
    static void releaseSpring(Player& player, SpringData* spring, int compressedCount, 
                              Screen& currentScreen, Game& game);
    
    // Handle spring entry for player
    static void handleSpringEntry(Player& player, SpringData* spring, const Point& position,
                                  Screen& currentScreen);
    
    // Handle spring compression (continuing movement on spring)
    static bool handleSpringCompression(Player& player, SpringData* spring, int currentIndex,
                                        int entryIndex, const Point& position, Screen& currentScreen);
    
    // Check if player should release spring (hit wall)
    static bool shouldReleaseAtWall(const Point& attemptedPos, SpringData* spring, 
                                    int entryIndex, Screen& currentScreen);
    
    // Handle perpendicular movement on spring
    static bool handlePerpendicularMovement(Player& player, SpringData* spring, 
                                            int moveDx, int moveDy, Screen& currentScreen, Game& game);
    
    // Convert boost direction to movement direction index
    static int boostDirToDirectionIndex(int boostDirX, int boostDirY);
};

class SpringViz {
    // Runtime-only visualization controller, not stored globally
public:
    // Compute collapsed cells along a linear run of '#' from contact point toward wall
    static std::vector<Point> computeCollapsed(const Screen& s, const Point& contact, int dirX, int dirY, int count);
};
