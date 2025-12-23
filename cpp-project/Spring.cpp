#include "Spring.h"
#include "Player.h"
#include "Game.h"
#include "Screen.h"
#include "Glyph.h"
#include "Obstacle.h"

// Static: Find spring at position
SpringData* SpringData::findAt(Screen& screen, const Point& p) {
    auto& data = screen.getDataMutable();
    for (auto& sp : data.springs) {
        if (sp.findCellIndex(p) != -1) {
            return &sp;
        }
    }
    return nullptr;
}

// SpringLogic class static method implementations

void SpringLogic::releaseSpring(Player& player, SpringData* spring, int compressedCount, 
                                Screen& currentScreen, Game& game) {
    if (!spring || compressedCount <= 0) return;
    
    Point position = player.getPosition();
    int currentRoomIdx = player.getRoomIdx();
    
    // Restore all spring cells visually
    for (const auto& cell : spring->getCells()) {
        currentScreen.setCharAt(cell, Glyph::Spring);
        currentScreen.refreshCell(cell);
    }
    
    // Launch boost: move at speed 'compressedCount' for 'compressedCount²' game cycles
    int releaseDirX = spring->getDirX();
    int releaseDirY = spring->getDirY();
    int speed = compressedCount;
    
    // FIRST: Move the player 'speed' cells immediately (this is the initial release jump)
    // This does NOT count as one of the compressedCount² cycles
    int actualSteps = 0;
    bool hitObstacle = false;
    
    for (int step = 0; step < speed; ++step) {
        Point next = position;
        next.setX(next.getX() + releaseDirX);
        next.setY(next.getY() + releaseDirY);
        
        // Check bounds
        if (next.getX() < 0 || next.getX() >= Screen::MAX_X || next.getY() < 0 || next.getY() >= Screen::MAX_Y) {
            hitObstacle = true;
            break;
        }
        
        wchar_t tile = currentScreen.getCharAt(next);
        
        // Check wall - stop at walls
        if (Glyph::isWall(tile)) {
            hitObstacle = true;
            break;
        }
        
        // Check obstacle - stop at obstacles
        if (Glyph::isObstacle(tile)) {
            Obstacle* obs = game.findObstacleAt(currentRoomIdx, next);
            if (obs) {
                if (obs->canPush(releaseDirX, releaseDirY, speed, game)) {
                    obs->applyPush(releaseDirX, releaseDirY, game);
                } else {
                    hitObstacle = true;
                    break;
                }
            } else {
                hitObstacle = true;
                break;
            }
        }
        
        // Check doors - stop at doors
        if (Glyph::isDoor(tile)) {
            hitObstacle = true;
            break;
        }
        
        // Move to next position
        currentScreen.refreshCell(position);
        position = next;
        actualSteps++;
        
        // Collect items while flying through
        if (Glyph::isKey(tile)) {
            if (player.canTakeObject()) {
                player.setCarried((char)tile);
                currentScreen.setCharAt(position, Glyph::Empty);
                currentScreen.refreshCell(position);
            }
        } else if (Glyph::isBomb(tile)) {
            if (player.canTakeObject()) {
                player.setCarried('@');
                currentScreen.setCharAt(position, Glyph::Empty);
                currentScreen.refreshCell(position);
            }
        } else if (Glyph::isTorch(tile)) {
            if (player.canTakeObject()) {
                player.setCarried('!');
                currentScreen.setCharAt(position, Glyph::Empty);
                currentScreen.refreshCell(position);
            }
        }
        
        // Check collision with other player - transfer boost including remaining initial jump
        for (auto& other : game.getPlayersMutable()) {
            if (&other == &player) continue;
            if (other.getRoomIdx() != currentRoomIdx) continue;
            Point op = other.getPosition();
            if (op.getX() == position.getX() && op.getY() == position.getY()) {
                // The other player should get:
                // 1. The remaining initial jump (speed - actualSteps cells)
                // 2. Plus all the boost cycles (compressedCount²)
                // But we can't easily simulate the remaining jump here, so we give them
                // the full cycles and let them complete the jump in their boost movement
                int totalCycles = compressedCount * compressedCount;
                other.inheritSpringLaunch(speed, totalCycles, releaseDirX, releaseDirY);
                break;
            }
        }
    }
    
    // Update player position
    player.setPosition(position);
    
    // If hit obstacle, complete STAY - no boost, no direction
    if (hitObstacle) {
        player.inheritSpringLaunch(0, 0, 0, 0);
        position.setDirection(MoveDirection::Stay);
        player.setPosition(position);
    } else {
        // SECOND: Set up boost for compressedCount² cycles (after the initial jump)
        int totalCycles = compressedCount * compressedCount;
        player.inheritSpringLaunch(speed, totalCycles, releaseDirX, releaseDirY);
    }
}

void SpringLogic::handleSpringEntry(Player& player, SpringData* spring, const Point& position,
Screen& currentScreen) {
    // Hide this cell
    currentScreen.setCharAt(position, Glyph::Empty);
    currentScreen.refreshCell(position);
}

bool SpringLogic::handleSpringCompression(Player& player, SpringData* spring, int currentIndex,
int entryIndex, const Point& position, Screen& currentScreen) {
    // Check direction: moving toward wall (decreasing index) or away
    if (currentIndex >= 0 && currentIndex < entryIndex) {
        // Moving toward wall (compressing)
        currentScreen.setCharAt(position, Glyph::Empty);
        currentScreen.refreshCell(position);
        return true; // Compression successful
    }
    return false; // Changed direction away from wall
}

bool SpringLogic::shouldReleaseAtWall(const Point& attemptedPos, SpringData* spring, 
int entryIndex, Screen& currentScreen) {
    // Check if we tried to move into the wall from cell 0
    if (entryIndex != 0) return false;
    
    wchar_t tile = currentScreen.getCharAt(attemptedPos);
    if (!Glyph::isWall(tile)) return false;
    
    // Check if it's the wall at the end of the spring
    Point wallPos = spring->getWallPos();
    return (attemptedPos.getX() == wallPos.getX() && attemptedPos.getY() == wallPos.getY());
}

bool SpringLogic::handlePerpendicularMovement(Player& player, SpringData* spring, 
int moveDx, int moveDy, Screen& currentScreen, Game& game) {
    int springDirX = spring->getDirX();
    int springDirY = spring->getDirY();
    
    
    // Check if movement is perpendicular (not along spring axis and not backward)
    bool movingPerpendicular = false;
    if (springDirX != 0 && moveDx == 0 && moveDy != 0) {
        // Horizontal spring, moving vertically
        movingPerpendicular = true;
    } else if (springDirY != 0 && moveDy == 0 && moveDx != 0) {
        // Vertical spring, moving horizontally
        movingPerpendicular = true;
    }
    
    return movingPerpendicular;
}

int SpringLogic::boostDirToDirectionIndex(int boostDirX, int boostDirY) {
    // Convert boost direction to direction index:
    // 0 = UP (y=-1), 1 = RIGHT (x=1), 2 = DOWN (y=1), 3 = LEFT (x=-1)
    if (boostDirX == 1 && boostDirY == 0) {
        return 1; // RIGHT
    } else if (boostDirX == -1 && boostDirY == 0) {
        return 3; // LEFT
    } else if (boostDirX == 0 && boostDirY == 1) {
        return 2; // DOWN
    } else if (boostDirX == 0 && boostDirY == -1) {
        return 0; // UP
    }
    return 4; // STAY (default)
}
