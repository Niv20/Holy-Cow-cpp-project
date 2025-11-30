#include "Player.h"
#include <cctype>
#include "Screen.h"
#include "Glyph.h"
#include <windows.h>
#include "Game.h"
#include "Obstacle.h"
#include "Spring.h"
#include <algorithm>

Player::Player(Point startPos, const char* keySet, wchar_t sym, int startRoom)
    : position(startPos), symbol(sym), currentRoomIdx(startRoom)
{ for (int i = 0; i < NUM_KEYS; i++) keys[i] = keySet[i]; }

void Player::draw() const {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos{ (SHORT)position.x, (SHORT)position.y };
    SetConsoleCursorPosition(hOut, pos);
    wchar_t out = symbol; DWORD written; WriteConsoleW(hOut, &out, 1, &written, nullptr);
}

void Player::erase(Screen& currentScreen) const { /* no direct erase */ }

void Player::inheritSpringLaunch(int speed, int ticks, int dirX, int dirY) {
    springBoostSpeed = speed;
    springBoostTicksLeft = ticks;
    boostDirX = dirX;
    boostDirY = dirY;
    currentSpring = nullptr;
    compressedCount = 0;
    // Clear movement direction to prevent interference
    position.setDirection(4); // STAY
}

void Player::releaseSpring(Screen& currentScreen, Game& game) {
    if (!currentSpring || compressedCount <= 0) return;
    
    // Restore all spring cells visually
    for (auto& cell : currentSpring->cells) {
        currentScreen.setCharAt(cell, Glyph::Spring);
        currentScreen.refreshCell(cell);
    }
    
    // Launch: move exactly 'speed' cells in one step, checking each cell
    int releaseDirX = currentSpring->dirX;
    int releaseDirY = currentSpring->dirY;
    int speed = compressedCount;
    
    // Try to move 'speed' cells, but stop at walls or bounds
    int actualSteps = 0;
    bool hitObstacle = false;
    
    for (int step = 0; step < speed; ++step) {
        Point next = position;
        next.x += releaseDirX;
        next.y += releaseDirY;
        
        // Check bounds
        if (next.x < 0 || next.x >= Screen::MAX_X || next.y < 0 || next.y >= Screen::MAX_Y) {
            hitObstacle = true;
            break;
        }
        
        // Check wall
        wchar_t tile = currentScreen.getCharAt(next);
        if (Glyph::isWall(tile)) {
            hitObstacle = true;
            break;
        }
        
        // Check obstacle
        if (Glyph::isObstacle(tile)) {
            Obstacle* obs = game.findObstacleAt(currentRoomIdx, next);
            if (obs) {
                if (obs->canPush(releaseDirX, releaseDirY, speed, game)) {
                    obs->applyPush(releaseDirX, releaseDirY, game);
                } else {
                    hitObstacle = true;
                    break;
                }
            }
        }
        
        // Move to next position
        currentScreen.refreshCell(position);
        position = next;
        actualSteps++;
        
        // Check collision with other player
        for (auto& other : game.getPlayersMutable()) {
            if (&other == this) continue;
            if (other.getRoomIdx() != currentRoomIdx) continue;
            Point op = other.getPosition();
            if (op.x == position.x && op.y == position.y) {
                int remainingTicks = compressedCount * compressedCount - actualSteps;
                if (remainingTicks > 0) {
                    other.inheritSpringLaunch(speed, remainingTicks, releaseDirX, releaseDirY);
                }
                break;
            }
        }
    }
    
    // If hit obstacle, complete STAY - no boost, no direction
    if (hitObstacle) {
        springBoostSpeed = 0;
        springBoostTicksLeft = 0;
        boostDirX = 0;
        boostDirY = 0;
        position.setDirection(4); // Force STAY
    } else {
        // Set up remaining boost
        springBoostSpeed = speed;
        int totalTicks = compressedCount * compressedCount;
        springBoostTicksLeft = totalTicks - actualSteps;
        if (springBoostTicksLeft < 0) springBoostTicksLeft = 0;
        boostDirX = releaseDirX;
        boostDirY = releaseDirY;
        position.setDirection(4); // Clear direction - boost controls movement
    }
    
    // Reset spring tracking
    currentSpring = nullptr;
    compressedCount = 0;
    entryIndex = -1;
}

void Player::move(Screen& currentScreen, Game& game) {
    Point originalPos = position;

    // Calculate cooperative force
    int appliedForce = getForce();
    for (auto& other : game.getPlayersMutable()) {
        if (&other == this) continue;
        if (other.getRoomIdx() != currentRoomIdx) continue;
        Point op = other.getPosition();
        bool adjacent = (abs(op.x - position.x) + abs(op.y - position.y)) == 1;
        if (adjacent && other.getPosition().diff_x == position.diff_x && other.getPosition().diff_y == position.diff_y) {
            appliedForce += other.getForce();
        }
    }

    // Apply movement with boost
    int moveDx = position.diff_x;
    int moveDy = position.diff_y;
    
    if (springBoostTicksLeft > 0) {
        // Cannot move backward against boost direction
        if ((moveDx != 0 && moveDx == -boostDirX) || (moveDy != 0 && moveDy == -boostDirY)) {
            moveDx = 0;
            moveDy = 0;
        }
        
        // Boost movement: speed cells per cycle in boost direction, plus lateral
        // First apply lateral movement (perpendicular to boost)
        if (boostDirX != 0) {
            // Boost is horizontal, allow vertical movement
            position.y += moveDy;
        } else if (boostDirY != 0) {
            // Boost is vertical, allow horizontal movement
            position.x += moveDx;
        }
        
        // Now apply boost movement (speed cells in boost direction)
        bool hitWall = false;
        for (int step = 0; step < springBoostSpeed && !hitWall; ++step) {
            Point next = position;
            next.x += boostDirX;
            next.y += boostDirY;
            
            // Check bounds
            if (next.x < 0 || next.x >= Screen::MAX_X || next.y < 0 || next.y >= Screen::MAX_Y) {
                hitWall = true;
                break;
            }
            
            wchar_t tile = currentScreen.getCharAt(next);
            if (Glyph::isWall(tile)) {
                hitWall = true;
                break;
            }
            
            currentScreen.refreshCell(position);
            position = next;
            
            // Check collision with other player during boost
            for (auto& other : game.getPlayersMutable()) {
                if (&other == this) continue;
                if (other.getRoomIdx() != currentRoomIdx) continue;
                Point op = other.getPosition();
                if (op.x == position.x && op.y == position.y) {
                    if (springBoostTicksLeft > 0) {
                        other.inheritSpringLaunch(springBoostSpeed, springBoostTicksLeft, boostDirX, boostDirY);
                    }
                    break;
                }
            }
        }
        
        // If hit wall, cancel boost and force STAY
        if (hitWall) {
            springBoostTicksLeft = 0;
            springBoostSpeed = 0;
            boostDirX = 0;
            boostDirY = 0;
            position.setDirection(4); // Force STAY
        } else {
            springBoostTicksLeft--;
            if (springBoostTicksLeft == 0) {
                springBoostSpeed = 0;
                boostDirX = 0;
                boostDirY = 0;
            }
        }
        
        // After boost movement, process tile at current position
        wchar_t tile = currentScreen.getCharAt(position);
        
        if (Glyph::isKey(tile)) {
            if (canTakeObject()) {
                setCarried((char)tile);
                currentScreen.setCharAt(position, Glyph::Empty);
                currentScreen.refreshCell(position);
            }
        } else if (Glyph::isBomb(tile)) {
            if (canTakeObject()) {
                setCarried('@');
                currentScreen.setCharAt(position, Glyph::Empty);
                currentScreen.refreshCell(position);
            }
        } else if (Glyph::isTorch(tile)) {
            if (canTakeObject()) {
                setCarried('!');
                currentScreen.setCharAt(position, Glyph::Empty);
                currentScreen.refreshCell(position);
            }
        }
        
        if (originalPos.x != position.x || originalPos.y != position.y) {
            currentScreen.refreshCell(originalPos);
        }
        draw();
        return; // Done with boost movement
    }
    
    // Normal movement (no boost)
    position.x += moveDx;
    position.y += moveDy;

    wchar_t tile = currentScreen.getCharAt(position);
    bool blocked = false;

    // Spring handling with pre-scanned data
    if (Glyph::isSpring(tile)) {
        SpringData* spring = game.findSpringAt(currentRoomIdx, position);
        
        if (!currentSpring) {
            // First entry to a spring
            if (spring) {
                currentSpring = spring;
                entryIndex = spring->findCellIndex(position);
                compressedCount = 1;
                
                // Hide this cell
                currentScreen.setCharAt(position, Glyph::Empty);
                currentScreen.refreshCell(position);
                
                // Check if we're already at the wall
                if (entryIndex == 0) {
                    // Entered at wall end - release immediately
                    releaseSpring(currentScreen, game);
                    currentScreen.refreshCell(originalPos);
                    draw();
                    return;
                }
            }
        } else if (spring == currentSpring) {
            // Continuing on same spring
            int currentIndex = spring->findCellIndex(position);
            
            // Check direction: moving toward wall (decreasing index) or away
            if (currentIndex >= 0 && currentIndex < entryIndex) {
                // Moving toward wall (compressing)
                compressedCount++;
                currentScreen.setCharAt(position, Glyph::Empty);
                currentScreen.refreshCell(position);
                
                // Check if reached wall (index 0)
                if (currentIndex == 0) {
                    // Reached wall - release!
                    releaseSpring(currentScreen, game);
                    currentScreen.refreshCell(originalPos);
                    draw();
                    return;
                }
            } else {
                // Changed direction - release
                position = originalPos;
                releaseSpring(currentScreen, game);
                currentScreen.refreshCell(originalPos);
                draw();
                return;
            }
        } else {
            // Moved to different spring - release old and start new
            releaseSpring(currentScreen, game);
        }
    } else {
        // Not on spring anymore
        if (currentSpring) {
            // Was on spring but moved off - release
            position = originalPos;
            releaseSpring(currentScreen, game);
            currentScreen.refreshCell(originalPos);
            draw();
            return;
        }
        
        // Handle other tiles
        if (Glyph::isObstacle(tile)) {
            Obstacle* obs = game.findObstacleAt(currentRoomIdx, position);
            if (!obs) {
                blocked = true;
            } else {
                int dx = position.diff_x;
                int dy = position.diff_y;
                if (dx == 0 && dy == 0) {
                    blocked = true;
                } else {
                    if (obs->canPush(dx, dy, appliedForce, game)) {
                        obs->applyPush(dx, dy, game);
                    } else {
                        blocked = true;
                    }
                }
            }
        } else if (Glyph::isKey(tile)) {
            if (canTakeObject()) {
                setCarried((char)tile);
                currentScreen.setCharAt(position, Glyph::Empty);
                currentScreen.refreshCell(position);
            }
        } else if (Glyph::isDoor(tile)) {
            if (getCarried() == std::tolower((unsigned char)tile)) {
                currentScreen.setCharAt(position, Glyph::Empty);
                currentScreen.refreshCell(position);
                setCarried(' ');
            } else {
                blocked = true;
            }
        } else if (Glyph::isBomb(tile)) {
            if (canTakeObject()) {
                setCarried('@');
                currentScreen.setCharAt(position, Glyph::Empty);
                currentScreen.refreshCell(position);
            }
        } else if (Glyph::isTorch(tile)) {
            if (canTakeObject()) {
                setCarried('!');
                currentScreen.setCharAt(position, Glyph::Empty);
                currentScreen.refreshCell(position);
            }
        } else if (Glyph::isWall(tile)) {
            blocked = true;
        } else if (Glyph::isRiddle(tile) || tile == Glyph::Empty) {
            // Allowed
        } else {
            blocked = true;
        }
    }

    if (blocked) {
        position = originalPos;
        position.setDirection(4);
        if (currentSpring) {
            releaseSpring(currentScreen, game);
        }
        return;
    }

    // Check for STAY key press while on spring
    if (currentSpring && position.diff_x == 0 && position.diff_y == 0) {
        position = originalPos;
        releaseSpring(currentScreen, game);
        currentScreen.refreshCell(originalPos);
        draw();
        return;
    }

    if (originalPos.x != position.x || originalPos.y != position.y) {
        currentScreen.refreshCell(originalPos);
    }

    // Item drop/pickup handling
    if (actionRequested) {
        char held = getCarried();
        if (held != ' ') {
            bool dropped = false;
            Point p = position;
            if (position.diff_x != 0 || position.diff_y != 0) {
                Point drop = p;
                if (position.diff_x == 1) drop.x = p.x - 1;
                else if (position.diff_x == -1) drop.x = p.x + 1;
                else if (position.diff_y == 1) drop.y = p.y - 1;
                else if (position.diff_y == -1) drop.y = p.y + 1;
                if (drop.x >= 0 && drop.x < Screen::MAX_X && drop.y >= 0 && drop.y < Screen::MAX_Y) {
                    if (currentScreen.getCharAt(drop) == Glyph::Empty) {
                        currentScreen.setCharAt(drop, (wchar_t)held);
                        currentScreen.refreshCell(drop);
                        setCarried(' ');
                        dropped = true;
                    }
                }
            }
            if (!dropped) {
                Point candidates[4] = { {p.x + 1, p.y}, {p.x - 1, p.y}, {p.x, p.y - 1}, {p.x, p.y + 1} };
                for (auto& q : candidates) {
                    if (q.x >= 0 && q.x < Screen::MAX_X && q.y >= 0 && q.y < Screen::MAX_Y && currentScreen.getCharAt(q) == Glyph::Empty) {
                        currentScreen.setCharAt(q, (wchar_t)held);
                        currentScreen.refreshCell(q);
                        setCarried(' ');
                        dropped = true;
                        break;
                    }
                }
            }
        } else {
            wchar_t under = currentScreen.getCharAt(position);
            if ((Glyph::isKey(under) || Glyph::isBomb(under) || Glyph::isTorch(under)) && canTakeObject()) {
                setCarried((char)under);
                currentScreen.setCharAt(position, Glyph::Empty);
                currentScreen.refreshCell(position);
            }
        }
        actionRequested = false;
    }

    draw();
}

void Player::handleKey(char key) {
    for (int i = 0; i < NUM_KEYS; i++) {
        if (std::tolower(key) == std::tolower(keys[i])) {
            position.setDirection(i);
            if (i == 5) actionRequested = true;
            return;
        }
    }
}

void Player::stop() {
    position.setDirection(4);
}