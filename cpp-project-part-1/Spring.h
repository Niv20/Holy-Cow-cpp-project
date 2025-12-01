#pragma once
#include "Point.h"
#include <vector>

// Forward declarations
class Screen;
class Game;
class Player;

// Represents a linear spring (# chars) adjacent to a wall at one end.
// Stores original cells so we can collapse visually and restore.
struct Spring {
    enum Orientation { Horizontal, Vertical };
    int roomIdx = -1;
    Point anchorStart; // first cell (closest to wall when compressed)
    int length = 0;
    Orientation orient = Horizontal;
    // Runtime compression state
    int compressedCount = 0; // how many chars currently compressed (player stands over them)
    bool releasing = false;
    int releaseTicksLeft = 0; // remaining boosted movement cycles
    int releaseSpeed = 0; // horizontal or vertical speed during release
};

struct SpringData {
    int roomIdx;
    std::vector<Point> cells; // all spring cells in order from wall
    int dirX; // direction away from wall (toward free end)
    int dirY;
    Point wallPos; // adjacent wall position
    
    SpringData(int room, const std::vector<Point>& springCells, int dx, int dy, Point wall)
        : roomIdx(room), cells(springCells), dirX(dx), dirY(dy), wallPos(wall) {}
    
    int length() const { return (int)cells.size(); }
    
    // Find which cell index player is on (returns -1 if not on this spring)
    int findCellIndex(const Point& p) const {
        for (int i = 0; i < (int)cells.size(); ++i) {
            if (cells[i].x == p.x && cells[i].y == p.y) return i;
        }
        return -1;
    }
};

// Spring logic helper functions
namespace SpringLogic {
    // Release spring and launch player
    void releaseSpring(Player& player, SpringData* spring, int compressedCount, 
                      Screen& currentScreen, Game& game);
    
    // Handle spring entry for player
    void handleSpringEntry(Player& player, SpringData* spring, const Point& position,
                          Screen& currentScreen);
    
    // Handle spring compression (continuing movement on spring)
    bool handleSpringCompression(Player& player, SpringData* spring, int currentIndex,
                                 int entryIndex, const Point& position, Screen& currentScreen);
    
    // Check if player should release spring (hit wall)
    bool shouldReleaseAtWall(const Point& attemptedPos, SpringData* spring, 
                            int entryIndex, Screen& currentScreen);
    
    // Handle perpendicular movement on spring
    bool handlePerpendicularMovement(Player& player, SpringData* spring, 
                                    int moveDx, int moveDy, Screen& currentScreen, Game& game);
    
    // Convert boost direction to movement direction index
    int boostDirToDirectionIndex(int boostDirX, int boostDirY);
}

class SpringViz {
    // Runtime-only visualization controller, not stored globally
public:
    // Compute collapsed cells along a linear run of '#' from contact point toward wall
    static std::vector<Point> computeCollapsed(const class Screen& s, const Point& contact, int dirX, int dirY, int count);
};
