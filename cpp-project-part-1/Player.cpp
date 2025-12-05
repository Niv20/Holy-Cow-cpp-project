#include "Player.h"
#include <cctype>
#include "Screen.h"
#include "Glyph.h"
#include <windows.h>
#include "Game.h"
#include "Obstacle.h"
#include "Spring.h"
#include "Switch.h"

/*      (__)
'\------(oo)    Constructor
  ||    (__)
  ||w--||                */

Player::Player(Point startPos, const char* keySet, wchar_t sym, int startRoom)
    : position(startPos), symbol(sym), currentRoomIdx(startRoom) { 
    for (int i = 0; i < NUM_KEYS; i++) 
        keys[i] = keySet[i]; 
}

/*      (__)
'\------(oo)    Basic functions
  ||    (__)
  ||w--||                    */

void Player::draw() const {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos{ (SHORT)position.x, (SHORT)position.y };
    SetConsoleCursorPosition(hOut, pos);
    wchar_t out = symbol; DWORD written; WriteConsoleW(hOut, &out, 1, &written, nullptr);
}

void Player::stop() {
    position.setDirection(4);
}

void Player::move(Screen& currentScreen, Game& game) {
    // Check if player reached final room - if so, prevent all movement
    if (currentRoomIdx == FINAL_ROOM_INDEX) {
        // Player reached final room - cannot move anymore
        position.setDirection(4); // Force STAY
        return;
    }
    
    Point originalPos = position;

    // Calculate cooperative force based on actual push direction
    int pushDx = (springBoostTicksLeft > 0) ? boostDirX : position.diff_x;
    int pushDy = (springBoostTicksLeft > 0) ? boostDirY : position.diff_y;
    int appliedForce = (springBoostTicksLeft > 0) ? springBoostSpeed : 1;
    for (auto& other : game.getPlayersMutable()) {
        if (&other == this) continue;
        if (other.getRoomIdx() != currentRoomIdx) continue;
        Point op = other.getPosition();
        bool adjacent = (abs(op.x - position.x) + abs(op.y - position.y)) == 1;
        if (!adjacent) continue;
        int otherPushDx = (other.getSpringBoostTicksLeft() > 0) ? other.getBoostDirX() : other.getPosition().diff_x;
        int otherPushDy = (other.getSpringBoostTicksLeft() > 0) ? other.getBoostDirY() : other.getPosition().diff_y;
        if (otherPushDx == pushDx && otherPushDy == pushDy && !(pushDx == 0 && pushDy == 0)) {
            int otherForce = (other.getSpringBoostTicksLeft() > 0) ? other.getSpringBoostSpeed() : 1;
            appliedForce += otherForce;
        }
    }

    // Apply movement with boost
    int moveDx = position.diff_x;
    int moveDy = position.diff_y;
    
    // Check if we're on spring and trying to move perpendicular
    if (currentSpring && (moveDx != 0 || moveDy != 0)) {
        if (SpringLogic::handlePerpendicularMovement(*this, currentSpring, moveDx, moveDy, currentScreen, game)) {
            // Release spring immediately and apply perpendicular movement
            releaseSpring(currentScreen, game);
            
            // Now apply the perpendicular movement (at speed 1)
            Point next = position;
            next.x += moveDx;
            next.y += moveDy;
            
            // Check if perpendicular move is valid
            if (next.x >= 0 && next.x < Screen::MAX_X && next.y >= 0 && next.y < Screen::MAX_Y) {
                wchar_t tile = currentScreen.getCharAt(next);
                if (!Glyph::isWall(tile)) {
                    currentScreen.refreshCell(position);
                    position = next;
                }
            }
            
            currentScreen.refreshCell(originalPos);
            draw();
            return;
        }
    }
    
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
            
            // Recompute cooperative force for this step (players may change adjacency during boost)
            int stepForce = springBoostSpeed; // self force during boost
            for (auto& other : game.getPlayersMutable()) {
                if (&other == this) continue;
                if (other.getRoomIdx() != currentRoomIdx) continue;
                Point op = other.getPosition();
                
                // Check adjacency relative to NEXT position (where we're moving to)
                // This is key: both players are moving forward, so we need to check
                // if they'll be adjacent at the obstacle, not at current positions
                bool adjacentToNext = (abs(op.x - next.x) + abs(op.y - next.y)) == 1;
                
                // Also check if other player will be adjacent to next after their boost step
                bool willBeAdjacentToNext = false;
                if (other.getSpringBoostTicksLeft() > 0) {
                    Point predictedOtherPos = op;
                    predictedOtherPos.x += other.getBoostDirX();
                    predictedOtherPos.y += other.getBoostDirY();
                    willBeAdjacentToNext = (abs(predictedOtherPos.x - next.x) + abs(predictedOtherPos.y - next.y)) == 1;
                }
                
                if (!adjacentToNext && !willBeAdjacentToNext) continue;
                
                int otherPushDx = (other.getSpringBoostTicksLeft() > 0) ? other.getBoostDirX() : other.getPosition().diff_x;
                int otherPushDy = (other.getSpringBoostTicksLeft() > 0) ? other.getBoostDirY() : other.getPosition().diff_y;
                if (otherPushDx == boostDirX && otherPushDy == boostDirY && !(boostDirX == 0 && boostDirY == 0)) {
                    int otherForce = (other.getSpringBoostTicksLeft() > 0) ? other.getSpringBoostSpeed() : 1;
                    stepForce += otherForce;
                }
            }
            
            wchar_t tile = currentScreen.getCharAt(next);
            
            // Stop at walls and non-passable tiles
            if (Glyph::isWall(tile)) {
                hitWall = true;
                break;
            }
            
            // Try to push obstacles during boost using cooperative force
            if (Glyph::isObstacle(tile)) {
                Obstacle* obs = game.findObstacleAt(currentRoomIdx, next);
                // Push obstacle with same speed as player boost
                if (obs && obs->canPush(boostDirX, boostDirY, stepForce, game, springBoostSpeed)) {
                    obs->applyPush(boostDirX, boostDirY, game, springBoostSpeed);
                    // After pushing, next cell should now be empty; proceed to move
                } else {
                    hitWall = true;
                    break;
                }
            }
            
            // Stop at doors (unless we can open them, but during boost we can't)
            if (Glyph::isDoor(tile)) {
                hitWall = true;
                break;
            }
            
            // Allow passing through: empty, keys, bombs, torches, riddles, switches
            // These are passable tiles
            
            currentScreen.refreshCell(position);
            position = next;
            
            // Collect items while flying through
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
                    // Remove bomb from active bombs list (reset timer)
                    game.removeBombAt(currentRoomIdx, position);
                }
            } else if (Glyph::isTorch(tile)) {
                if (canTakeObject()) {
                    setCarried('!');
                    currentScreen.setCharAt(position, Glyph::Empty);
                    currentScreen.refreshCell(position);
                }
            }
            
            // Check collision with other player during boost
            for (auto& other : game.getPlayersMutable()) {
                if (&other == this) continue;
                if (other.getRoomIdx() != currentRoomIdx) continue;
                Point op = other.getPosition();
                if (op.x == position.x && op.y == position.y) {
                    // Transfer remaining cycles AFTER this cycle to the other player
                    // so both will have the same ticks next frame
                    int remainingAfterThisCycle = springBoostTicksLeft - 1;
                    if (remainingAfterThisCycle < 0) remainingAfterThisCycle = 0;
                    if (remainingAfterThisCycle > 0) {
                        other.inheritSpringLaunch(springBoostSpeed, remainingAfterThisCycle, boostDirX, boostDirY);
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
                // Boost ended - continue at speed 1 in the same direction
                springBoostSpeed = 0;
                // Keep the direction active: convert boost direction to direction index
                int dirIndex = SpringLogic::boostDirToDirectionIndex(boostDirX, boostDirY);
                position.setDirection(dirIndex);
                // Clear boost direction variables
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
                // Remove bomb from active bombs list (reset timer)
                game.removeBombAt(currentRoomIdx, position);
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
    // Calculate target position
    Point targetPos = originalPos;
    targetPos.x += moveDx;
    targetPos.y += moveDy;
    
    bool blocked = false;
    
    // Check bounds
    if (targetPos.x < 0 || targetPos.x >= Screen::MAX_X || 
        targetPos.y < 0 || targetPos.y >= Screen::MAX_Y) {
        blocked = true;
    } else {
        // Check what's at target position BEFORE moving
        wchar_t tile = currentScreen.getCharAt(targetPos);
        
        // Check for switch first
        if (Glyph::isSwitch(tile)) {
            SwitchData* sw = game.findSwitchAt(currentRoomIdx, targetPos);
            if (sw) {
                // Toggle the switch
                sw->toggle();
                
                // Update screen with new switch state
                currentScreen.setCharAt(targetPos, sw->getDisplayChar());
                currentScreen.refreshCell(targetPos);
                
                // Push player back one step in opposite direction
                Point pushBackPos = originalPos;
                pushBackPos.x -= moveDx;
                pushBackPos.y -= moveDy;
                
                // Make sure pushback position is valid
                if (pushBackPos.x >= 0 && pushBackPos.x < Screen::MAX_X &&
                    pushBackPos.y >= 0 && pushBackPos.y < Screen::MAX_Y) {
                    wchar_t pushBackTile = currentScreen.getCharAt(pushBackPos);
                    if (!Glyph::isWall(pushBackTile)) {
                        position = pushBackPos;
                    }
                    // else stay at original position (position already = originalPos)
                }
                // else stay at original position
                
                // Force STAY - stop movement
                position.setDirection(4);
                
                // Refresh cells
                currentScreen.refreshCell(originalPos);
                if (position.x != originalPos.x || position.y != originalPos.y) {
                    currentScreen.refreshCell(position);
                }
                
                draw();
                return;
            }
        }

        // Spring handling with pre-scanned data
        if (Glyph::isSpring(tile)) {
            SpringData* spring = game.findSpringAt(currentRoomIdx, targetPos);
            
            // Move to target first
            position = targetPos;
            
            if (!currentSpring) {
                // First entry to a spring
                if (spring) {
                    currentSpring = spring;
                    entryIndex = spring->findCellIndex(position);
                    compressedCount = 1;
                    
                    // Hide this cell
                    SpringLogic::handleSpringEntry(*this, spring, position, currentScreen);
                }
            } else if (spring == currentSpring) {
                // Continuing on same spring
                int currentIndex = spring->findCellIndex(position);
                
                // Check direction: moving toward wall (decreasing index) or away
                if (currentIndex >= 0 && currentIndex < entryIndex) {
                    // Moving toward wall (compressing)
                    if (SpringLogic::handleSpringCompression(*this, spring, currentIndex, entryIndex, position, currentScreen)) {
                        compressedCount++;
                        entryIndex = currentIndex;
                    }
                } else {
                    // Changed direction away from wall - release
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
            // Not on spring anymore - check if we should release
            if (currentSpring) {
                // We tried to move off the spring
                // Check if we're trying to move toward the wall from cell 0 (fully compressed)
                bool tryingToReachWall = SpringLogic::shouldReleaseAtWall(targetPos, currentSpring, entryIndex, currentScreen);
                
                // If we tried to reach the wall from the last spring cell, release the spring
                if (tryingToReachWall) {
                    // Perfect! We compressed all the way and tried to move into the wall
                    // Stay at original position
                    releaseSpring(currentScreen, game);
                    currentScreen.refreshCell(originalPos);
                    draw();
                    return;
                }
                
                // Otherwise, we moved off the spring in a different direction - release
                releaseSpring(currentScreen, game);
            }
            
            // Handle other tiles
            if (Glyph::isSpecialDoor(tile)) {
                SpecialDoor* door = game.findSpecialDoorAt(currentRoomIdx, targetPos);
                if (door && getCarried() != ' ' && door->useKey(Key(getCarried()))) {
                    // Consume the key but do NOT remove door or pass through yet
                    setCarried(' ');
                    blocked = true; // still blocked until all conditions met
                } else {
                    blocked = true; // Always blocked by special doors until opened by game logic
                }
            } else if (Glyph::isObstacle(tile)) {
                // IMPORTANT: Check for obstacle BEFORE moving player
                Obstacle* obs = game.findObstacleAt(currentRoomIdx, targetPos);
                if (!obs) {
                    blocked = true;
                } else {
                    if (moveDx == 0 && moveDy == 0) {
                        blocked = true;
                    } else {
                        // Try to push obstacle
                        if (obs->canPush(moveDx, moveDy, appliedForce, game)) {
                            // Push succeeds: first push obstacle, then move player
                            obs->applyPush(moveDx, moveDy, game);
                            // Rescan obstacles to refresh instances across rooms after movement
                            game.rescanObstacles();
                            // Now move player to target (which should now be empty)
                            wchar_t afterPush = currentScreen.getCharAt(targetPos);
                            if (afterPush == Glyph::Empty) {
                                position = targetPos;
                            } else {
                                // Race condition or bug: target not empty after push
                                blocked = true;
                            }
                        } else {
                            // Not enough force or blocked
                            blocked = true;
                        }
                    }
                }
            } else if (Glyph::isWall(tile)) {
                blocked = true;
            } else if (Glyph::isDoor(tile)) {
                if (getCarried() == std::tolower((unsigned char)tile)) {
                    // Can open door
                    position = targetPos;
                    currentScreen.setCharAt(position, Glyph::Empty);
                    currentScreen.refreshCell(position);
                    setCarried(' ');
                } else {
                    blocked = true;
                }
            } else if (Glyph::isKey(tile)) {
                position = targetPos;
                if (canTakeObject()) {
                    setCarried((char)tile);
                    currentScreen.setCharAt(position, Glyph::Empty);
                    currentScreen.refreshCell(position);
                }
            } else if (Glyph::isBomb(tile)) {
                position = targetPos;
                if (canTakeObject()) {
                    setCarried('@');
                    currentScreen.setCharAt(position, Glyph::Empty);
                    currentScreen.refreshCell(position);
                    // Remove bomb from active bombs list (reset timer)
                    game.removeBombAt(currentRoomIdx, position);
                }
            } else if (Glyph::isTorch(tile)) {
                position = targetPos;
                if (canTakeObject()) {
                    setCarried('!');
                    currentScreen.setCharAt(position, Glyph::Empty);
                    currentScreen.refreshCell(position);
                }
            } else if (Glyph::isRiddle(tile) || tile == Glyph::Empty) {
                // Allowed - move to target
                position = targetPos;
            } else {
                blocked = true;
            }
        }
    }

    if (blocked) {
        position = originalPos;
        // DO NOT reset direction here - preserve diff_x/diff_y for cooperative push
        // position.setDirection(4);
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
                        // If dropped bomb, activate it (start countdown)
                        if (held == '@') {
                            game.placeBomb(currentRoomIdx, drop, 5);
                        }
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
                        // If dropped bomb, activate it (start countdown)
                        if (held == '@') {
                            game.placeBomb(currentRoomIdx, q, 5);
                        }
                        break;
                    }
                }
            }
        } else {
            wchar_t under = currentScreen.getCharAt(position);
            // Can only pick up items if not already activated (bombs on ground are activated and counting down)
            // Keys and torches can be picked up multiple times, but bombs cannot be picked up once placed
            if ((Glyph::isKey(under) || Glyph::isTorch(under)) && canTakeObject()) {
                setCarried((char)under);
                currentScreen.setCharAt(position, Glyph::Empty);
                currentScreen.refreshCell(position);
            }
            // Bombs can only be picked up initially from map, not after being placed/activated
            // (Fresh bombs from map are picked up automatically when walked over, not via action key)
        }
        actionRequested = false;
    }

    // After movement and item handling, auto-use key on adjacent special door
    if (getCarried() != ' ') {
        static const int adjDx[4] = { 1, -1, 0, 0 };
        static const int adjDy[4] = { 0, 0, 1, -1 };
        for (int i = 0; i < 4; ++i) {
            Point adj{ position.x + adjDx[i], position.y + adjDy[i] };
            if (adj.x < 0 || adj.x >= Screen::MAX_X || adj.y < 0 || adj.y >= Screen::MAX_Y) continue;
            wchar_t ch = currentScreen.getCharAt(adj);
            if (Glyph::isSpecialDoor(ch)) {
                auto* door = game.findSpecialDoorAt(currentRoomIdx, adj);
                if (door && door->useKey(Key(getCarried()))) {
                    setCarried(' ');
                    break; // consumed one key; door remains until conditions met
                }
            }
        }
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

/*      (__)
'\------(oo)    Spring functions
  ||    (__)
  ||w--||                     */

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
    SpringLogic::releaseSpring(*this, currentSpring, compressedCount, currentScreen, game);

    // Reset spring tracking
    currentSpring = nullptr;
    compressedCount = 0;
    entryIndex = -1;
}