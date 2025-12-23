#include "Player.h"
#include <cctype>
#include "Screen.h"
#include "Glyph.h"
#include <windows.h>
#include "Game.h"
#include "Obstacle.h"
#include "Spring.h"
#include "Switch.h"
#include "SpecialDoor.h"
#include "DarkRoom.h"

namespace {
    constexpr int ADJACENT_DISTANCE = 1;
    constexpr int DEFAULT_PUSH_FORCE = 1;
    constexpr int ADJACENT_OFFSETS_COUNT = 4;
    constexpr int ACTION_KEY_INDEX = 5;
    constexpr int BOMB_DEFAULT_DELAY = 5;
    constexpr char NO_CARRIED_ITEM = ' ';
}

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
    COORD pos{ (SHORT)position.getX(), (SHORT)position.getY() };
    SetConsoleCursorPosition(hOut, pos);
    wchar_t out = symbol; DWORD written; WriteConsoleW(hOut, &out, 1, &written, nullptr);
}

void Player::stop() {
    position.setDirection(MoveDirection::Stay);
}

void Player::move(Screen& currentScreen, Game& game) {
    // Reset moved flag at start
    movedThisFrame = false;
    
    // Check if player reached final room - if so, prevent all movement
    if (currentRoomIdx == FINAL_ROOM_INDEX) {
        position.setDirection(MoveDirection::Stay);
        return;
    }
    
    Point originalPos = position;
    int appliedForce = calculateCooperativeForce(game);
    int moveDx = position.getDiffX();
    int moveDy = position.getDiffY();
    
    // Check if we're on spring and trying to move perpendicular
    if (handleSpringPerpendicularMove(currentScreen, game, originalPos, moveDx, moveDy)) {
        return;
    }
    
    // Handle boost movement from spring release
    if (processBoostMovement(currentScreen, game, originalPos)) {
        return;
    }
    
    // Normal movement (no boost)
    processNormalMovement(currentScreen, game, originalPos, appliedForce);
    
    // Item drop/pickup handling
    handleItemAction(currentScreen, game);
    
    // Auto-use key on adjacent special door
    handleAdjacentDoorKeys(currentScreen, game);
    
    // Check for teleportation through opened special door
    handleTeleportation(currentScreen, game);
}

void Player::handleKey(char key) {
    for (int i = 0; i < NUM_KEYS; i++) {
        if (std::tolower(key) == std::tolower(keys[i])) {
            if (i == ACTION_KEY_INDEX) {
                // Action key: don't change direction, just request action
                actionRequested = true;
            } else {
                // Movement key: set direction
                position.setDirection(intToMoveDirection(i));
            }
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
    position.setDirection(MoveDirection::Stay);
}

void Player::releaseSpring(Screen& currentScreen, Game& game) {
    SpringLogic::releaseSpring(*this, currentSpring, compressedCount, currentScreen, game);

    // Reset spring tracking
    currentSpring = nullptr;
    compressedCount = 0;
    entryIndex = -1;
}

/*      (__)
'\------(oo)    Movement helper functions
  ||    (__)
  ||w--||                              */

int Player::calculateCooperativeForce(Game& game) const {
    int pushDx = (springBoostTicksLeft > 0) ? boostDirX : position.getDiffX();
    int pushDy = (springBoostTicksLeft > 0) ? boostDirY : position.getDiffY();
    int appliedForce = (springBoostTicksLeft > 0) ? springBoostSpeed : DEFAULT_PUSH_FORCE;
    
    for (auto& other : game.getPlayersMutable()) {
        if (&other == this) continue;
        if (other.getRoomIdx() != currentRoomIdx) continue;
        Point op = other.getPosition();
        bool adjacent = (abs(op.getX() - position.getX()) + abs(op.getY() - position.getY())) == ADJACENT_DISTANCE;
        if (!adjacent) continue;
        int otherPushDx = (other.getSpringBoostTicksLeft() > 0) ? other.getBoostDirX() : other.getPosition().getDiffX();
        int otherPushDy = (other.getSpringBoostTicksLeft() > 0) ? other.getBoostDirY() : other.getPosition().getDiffY();
        if (otherPushDx == pushDx && otherPushDy == pushDy && !(pushDx == 0 && pushDy == 0)) {
            int otherForce = (other.getSpringBoostTicksLeft() > 0) ? other.getSpringBoostSpeed() : DEFAULT_PUSH_FORCE;
            appliedForce += otherForce;
        }
    }
    return appliedForce;
}

bool Player::handleSpringPerpendicularMove(Screen& currentScreen, Game& game, const Point& originalPos, int moveDx, int moveDy) {
    if (!currentSpring || (moveDx == 0 && moveDy == 0)) {
        return false;
    }
    
    if (!SpringLogic::handlePerpendicularMovement(*this, currentSpring, moveDx, moveDy, currentScreen, game)) {
        return false;
    }
    
    // Release spring immediately and apply perpendicular movement
    releaseSpring(currentScreen, game);
    
    // Now apply the perpendicular movement (at speed 1)
    Point next = position;
    next.setX(next.getX() + moveDx);
    next.setY(next.getY() + moveDy);
    
    // Check if perpendicular move is valid
    if (next.getX() >= 0 && next.getX() < Screen::MAX_X && next.getY() >= 0 && next.getY() < Screen::MAX_Y) {
        wchar_t tile = currentScreen.getCharAt(next);
        // Check if another player is at the target position
        bool playerBlocking = false;
        for (const auto& other : game.getPlayers()) {
            if (&other == this) continue;
            if (other.getRoomIdx() != currentRoomIdx) continue;
            Point op = other.getPosition();
            if (op.getX() == next.getX() && op.getY() == next.getY()) {
                playerBlocking = true;
                break;
            }
        }
        if (!Glyph::isWall(tile) && !playerBlocking) {
            currentScreen.refreshCell(position);
            position = next;
        }
    }
    
    currentScreen.refreshCell(originalPos);
    return true;
}

void Player::tryCollectItem(Screen& currentScreen, Game& game, wchar_t tile) {
    if (Glyph::isKey(tile)) {
        if (canTakeObject()) {
            setCarried((char)tile);
            currentScreen.setCharAt(position, Glyph::Empty);
            currentScreen.refreshCell(position);
        }
    } else if (Glyph::isBomb(tile)) {
        if (canTakeObject()) {
            setCarried(static_cast<char>(Glyph::Bomb));
            currentScreen.setCharAt(position, Glyph::Empty);
            currentScreen.refreshCell(position);
            game.removeBombAt(currentRoomIdx, position);
        }
    } else if (Glyph::isTorch(tile)) {
        if (canTakeObject()) {
            setCarried(static_cast<char>(Glyph::Torch));
            currentScreen.setCharAt(position, Glyph::Empty);
            currentScreen.refreshCell(position);
        }
    }
}

bool Player::processBoostMovement(Screen& currentScreen, Game& game, const Point& originalPos) {
    if (springBoostTicksLeft <= 0) {
        return false;
    }
    
    int moveDx = position.getDiffX();
    int moveDy = position.getDiffY();
    
    // Cannot move backward against boost direction
    if ((moveDx != 0 && moveDx == -boostDirX) || (moveDy != 0 && moveDy == -boostDirY)) {
        moveDx = 0;
        moveDy = 0;
    }
    
    // Boost movement: speed cells per cycle in boost direction, plus lateral
    // First apply lateral movement (perpendicular to boost)
    Point lateralTarget = position;
    if (boostDirX != 0) {
        lateralTarget.setY(lateralTarget.getY() + moveDy);
    } else if (boostDirY != 0) {
        lateralTarget.setX(lateralTarget.getX() + moveDx);
    }
    
    bool lateralBlocked = false;
    if (lateralTarget.getX() != position.getX() || lateralTarget.getY() != position.getY()) {
        for (const auto& other : game.getPlayers()) {
            if (&other == this) continue;
            if (other.getRoomIdx() != currentRoomIdx) continue;
            Point op = other.getPosition();
            if (op.getX() == lateralTarget.getX() && op.getY() == lateralTarget.getY()) {
                lateralBlocked = true;
                break;
            }
        }
    }
    
    if (!lateralBlocked) {
        if (boostDirX != 0) {
            position.setY(position.getY() + moveDy);
        } else if (boostDirY != 0) {
            position.setX(position.getX() + moveDx);
        }
    }
    
    // Now apply boost movement (speed cells in boost direction)
    bool hitWall = false;
    for (int step = 0; step < springBoostSpeed && !hitWall; ++step) {
        Point next = position;
        next.setX(next.getX() + boostDirX);
        next.setY(next.getY() + boostDirY);
        
        // Check bounds
        if (next.getX() < 0 || next.getX() >= Screen::MAX_X || next.getY() < 0 || next.getY() >= Screen::MAX_Y) {
            hitWall = true;
            break;
        }
        
        // Recompute cooperative force for this step
        int stepForce = springBoostSpeed;
        for (auto& other : game.getPlayersMutable()) {
            if (&other == this) continue;
            if (other.getRoomIdx() != currentRoomIdx) continue;
            Point op = other.getPosition();
            
            bool adjacentToNext = (abs(op.getX() - next.getX()) + abs(op.getY() - next.getY())) == ADJACENT_DISTANCE;
            
            bool willBeAdjacentToNext = false;
            if (other.getSpringBoostTicksLeft() > 0) {
                Point predictedOtherPos = op;
                predictedOtherPos.setX(predictedOtherPos.getX() + other.getBoostDirX());
                predictedOtherPos.setY(predictedOtherPos.getY() + other.getBoostDirY());
                willBeAdjacentToNext = (abs(predictedOtherPos.getX() - next.getX()) + abs(predictedOtherPos.getY() - next.getY())) == ADJACENT_DISTANCE;
            }
            
            if (!adjacentToNext && !willBeAdjacentToNext) continue;
            
            int otherPushDx = (other.getSpringBoostTicksLeft() > 0) ? other.getBoostDirX() : other.getPosition().getDiffX();
            int otherPushDy = (other.getSpringBoostTicksLeft() > 0) ? other.getBoostDirY() : other.getPosition().getDiffY();
            if (otherPushDx == boostDirX && otherPushDy == boostDirY && !(boostDirX == 0 && boostDirY == 0)) {
                int otherForce = (other.getSpringBoostTicksLeft() > 0) ? other.getSpringBoostSpeed() : DEFAULT_PUSH_FORCE;
                stepForce += otherForce;
            }
        }
        
        wchar_t tile = currentScreen.getCharAt(next);
        
        if (Glyph::isWall(tile)) {
            hitWall = true;
            break;
        }
        
        if (Glyph::isObstacle(tile)) {
            Obstacle* obs = game.findObstacleAt(currentRoomIdx, next);
            if (obs && obs->canPush(boostDirX, boostDirY, stepForce, game, springBoostSpeed)) {
                obs->applyPush(boostDirX, boostDirY, game, springBoostSpeed);
            } else {
                hitWall = true;
                break;
            }
        }
        
        if (Glyph::isDoor(tile)) {
            hitWall = true;
            break;
        }
        
        currentScreen.refreshCell(position);
        position = next;
        
        tryCollectItem(currentScreen, game, tile);
        
        // Check collision with other player during boost
        for (auto& other : game.getPlayersMutable()) {
            if (&other == this) continue;
            if (other.getRoomIdx() != currentRoomIdx) continue;
            Point op = other.getPosition();
            if (op.getX() == position.getX() && op.getY() == position.getY()) {
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
        position.setDirection(MoveDirection::Stay);
    } else {
        springBoostTicksLeft--;
        if (springBoostTicksLeft == 0) {
            springBoostSpeed = 0;
            int dirIndex = SpringLogic::boostDirToDirectionIndex(boostDirX, boostDirY);
            position.setDirection(intToMoveDirection(dirIndex));
            boostDirX = 0;
            boostDirY = 0;
        }
    }
    
    // After boost movement, process tile at current position
    wchar_t tile = currentScreen.getCharAt(position);
    tryCollectItem(currentScreen, game, tile);
    
    if (originalPos.getX() != position.getX() || originalPos.getY() != position.getY()) {
        currentScreen.refreshCell(originalPos);
    }
    return true;
}

bool Player::processNormalMovement(Screen& currentScreen, Game& game, const Point& originalPos, int appliedForce) {
    int moveDx = position.getDiffX();
    int moveDy = position.getDiffY();
    
    Point targetPos = originalPos;
    targetPos.setX(targetPos.getX() + moveDx);
    targetPos.setY(targetPos.getY() + moveDy);
    
    bool blocked = false;
    
    // Check bounds
    if (targetPos.getX() < 0 || targetPos.getX() >= Screen::MAX_X || 
        targetPos.getY() < 0 || targetPos.getY() >= Screen::MAX_Y) {
        blocked = true;
    }
    // Check if trying to enter dark zone without torch
    else if (!DarkRoomManager::canEnterPosition(currentScreen, *this, targetPos, game.getPlayers(), currentRoomIdx)) {
        blocked = true;
    }
    // Check if another player is at the target position
    else {
        for (const auto& other : game.getPlayers()) {
            if (&other == this) continue;
            if (other.getRoomIdx() != currentRoomIdx) continue;
            Point op = other.getPosition();
            if (op.getX() == targetPos.getX() && op.getY() == targetPos.getY()) {
                blocked = true;
                break;
            }
        }
    }
    
    if (!blocked) {
        wchar_t tile = currentScreen.getCharAt(targetPos);
        
        // Check for switch first
        if (Glyph::isSwitch(tile)) {
            SwitchData* sw = game.findSwitchAt(currentRoomIdx, targetPos);
            if (sw) {
                sw->toggle();
                currentScreen.setCharAt(targetPos, sw->getDisplayChar());
                currentScreen.refreshCell(targetPos);
                
                Point pushBackPos = originalPos;
                pushBackPos.setX(pushBackPos.getX() - moveDx);
                pushBackPos.setY(pushBackPos.getY() - moveDy);
                
                if (pushBackPos.getX() >= 0 && pushBackPos.getX() < Screen::MAX_X &&
                    pushBackPos.getY() >= 0 && pushBackPos.getY() < Screen::MAX_Y) {
                    wchar_t pushBackTile = currentScreen.getCharAt(pushBackPos);
                    if (!Glyph::isWall(pushBackTile)) {
                        position = pushBackPos;
                    }
                }
                
                position.setDirection(MoveDirection::Stay);
                currentScreen.refreshCell(originalPos);
                if (position.getX() != originalPos.getX() || position.getY() != originalPos.getY()) {
                    currentScreen.refreshCell(position);
                }
                return true;
            }
        }

        // Spring handling
        if (Glyph::isSpring(tile)) {
            SpringData* spring = game.findSpringAt(currentRoomIdx, targetPos);
            position = targetPos;
            
            if (!currentSpring) {
                if (spring) {
                    currentSpring = spring;
                    entryIndex = spring->findCellIndex(position);
                    compressedCount = 1;
                    SpringLogic::handleSpringEntry(*this, spring, position, currentScreen);
                }
            } else if (spring == currentSpring) {
                int currentIndex = spring->findCellIndex(position);
                if (currentIndex >= 0 && currentIndex < entryIndex) {
                    if (SpringLogic::handleSpringCompression(*this, spring, currentIndex, entryIndex, position, currentScreen)) {
                        compressedCount++;
                        entryIndex = currentIndex;
                    }
                } else {
                    position = originalPos;
                    releaseSpring(currentScreen, game);
                    currentScreen.refreshCell(originalPos);
                    return true;
                }
            } else {
                releaseSpring(currentScreen, game);
            }
        } else {
            if (currentSpring) {
                bool tryingToReachWall = SpringLogic::shouldReleaseAtWall(targetPos, currentSpring, entryIndex, currentScreen);
                if (tryingToReachWall) {
                    releaseSpring(currentScreen, game);
                    currentScreen.refreshCell(originalPos);
                    return true;
                }
                releaseSpring(currentScreen, game);
            }
            
            // Handle other tiles
            if (Glyph::isSpecialDoor(tile)) {
                SpecialDoor* door = game.findSpecialDoorAt(currentRoomIdx, targetPos);
                if (door) {
                    if (door->isOpen() && door->getTargetRoomIdx() >= 0) {
                        position = targetPos;
                        blocked = false;
                    } else if (getCarried() != NO_CARRIED_ITEM && door->useKey(Key(getCarried()))) {
                        setCarried(NO_CARRIED_ITEM);
                        blocked = true;
                    } else {
                        blocked = true;
                    }
                } else {
                    blocked = true;
                }
            } else if (Glyph::isObstacle(tile)) {
                Obstacle* obs = game.findObstacleAt(currentRoomIdx, targetPos);
                if (!obs) {
                    blocked = true;
                } else {
                    if (moveDx == 0 && moveDy == 0) {
                        blocked = true;
                    } else {
                        if (obs->canPush(moveDx, moveDy, appliedForce, game)) {
                            obs->applyPush(moveDx, moveDy, game);
                            game.rescanObstacles();
                            wchar_t afterPush = currentScreen.getCharAt(targetPos);
                            if (afterPush == Glyph::Empty) {
                                position = targetPos;
                            } else {
                                blocked = true;
                            }
                        } else {
                            blocked = true;
                        }
                    }
                }
            } else if (Glyph::isWall(tile)) {
                blocked = true;
            } else if (Glyph::isDoor(tile)) {
                if (getCarried() == std::tolower((unsigned char)tile)) {
                    position = targetPos;
                    currentScreen.setCharAt(position, Glyph::Empty);
                    currentScreen.refreshCell(position);
                    setCarried(NO_CARRIED_ITEM);
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
                    setCarried(static_cast<char>(Glyph::Bomb));
                    currentScreen.setCharAt(position, Glyph::Empty);
                    currentScreen.refreshCell(position);
                    game.removeBombAt(currentRoomIdx, position);
                }
            } else if (Glyph::isTorch(tile)) {
                position = targetPos;
                if (canTakeObject()) {
                    setCarried(static_cast<char>(Glyph::Torch));
                    currentScreen.setCharAt(position, Glyph::Empty);
                    currentScreen.refreshCell(position);
                }
            } else if (Glyph::isRiddle(tile) || tile == Glyph::Empty || Glyph::isPressureButton(tile)) {
                position = targetPos;
            } else {
                blocked = true;
            }
        }
    }

    if (blocked) {
        position = originalPos;
        if (currentSpring) {
            releaseSpring(currentScreen, game);
        }
        return false;
    }

    // Check for STAY key press while on spring
    if (currentSpring && position.getDiffX() == 0 && position.getDiffY() == 0) {
        position = originalPos;
        releaseSpring(currentScreen, game);
        currentScreen.refreshCell(originalPos);
        return false;
    }

    if (originalPos.getX() != position.getX() || originalPos.getY() != position.getY()) {
        currentScreen.refreshCell(originalPos);
    }
    
    return true;
}

void Player::handleItemAction(Screen& currentScreen, Game& game) {
    if (!actionRequested) {
        return;
    }
    
    char held = getCarried();
    if (held != NO_CARRIED_ITEM) {
        bool dropped = false;
        Point p = position;
        if (position.getDiffX() != 0 || position.getDiffY() != 0) {
            Point drop = p;
            if (position.getDiffX() == 1) drop.setX(p.getX() - 1);
            else if (position.getDiffX() == -1) drop.setX(p.getX() + 1);
            else if (position.getDiffY() == 1) drop.setY(p.getY() - 1);
            else if (position.getDiffY() == -1) drop.setY(p.getY() + 1);
            if (drop.getX() >= 0 && drop.getX() < Screen::MAX_X && drop.getY() >= 0 && drop.getY() < Screen::MAX_Y) {
                if (currentScreen.getCharAt(drop) == Glyph::Empty) {
                    currentScreen.setCharAt(drop, (wchar_t)held);
                    currentScreen.refreshCell(drop);
                    setCarried(NO_CARRIED_ITEM);
                    dropped = true;
                    if (held == static_cast<char>(Glyph::Bomb)) {
                        game.placeBomb(currentRoomIdx, drop, BOMB_DEFAULT_DELAY);
                    }
                }
            }
        }
        if (!dropped) {
            Point candidates[ADJACENT_OFFSETS_COUNT] = { Point(p.getX(), p.getY() - 1), Point(p.getX(), p.getY() + 1), Point(p.getX() + 1, p.getY()), Point(p.getX() - 1, p.getY()) };
            for (auto& q : candidates) {
                if (q.getX() >= 0 && q.getX() < Screen::MAX_X && q.getY() >= 0 && q.getY() < Screen::MAX_Y && currentScreen.getCharAt(q) == Glyph::Empty) {
                    currentScreen.setCharAt(q, (wchar_t)held);
                    currentScreen.refreshCell(q);
                    setCarried(NO_CARRIED_ITEM);
                    dropped = true;
                    if (held == static_cast<char>(Glyph::Bomb)) {
                        game.placeBomb(currentRoomIdx, q, BOMB_DEFAULT_DELAY);
                    }
                    break;
                }
            }
        }
    } else {
        wchar_t under = currentScreen.getCharAt(position);
        if ((Glyph::isKey(under) || Glyph::isTorch(under)) && canTakeObject()) {
            setCarried((char)under);
            currentScreen.setCharAt(position, Glyph::Empty);
            currentScreen.refreshCell(position);
        }
    }
    actionRequested = false;
}

void Player::handleAdjacentDoorKeys(Screen& currentScreen, Game& game) {
    if (getCarried() == NO_CARRIED_ITEM) {
        return;
    }
    
    static const int adjDx[ADJACENT_OFFSETS_COUNT] = { 1, -1, 0, 0 };
    static const int adjDy[ADJACENT_OFFSETS_COUNT] = { 0, 0, 1, -1 };
    for (int i = 0; i < ADJACENT_OFFSETS_COUNT; ++i) {
        Point adj(position.getX() + adjDx[i], position.getY() + adjDy[i]);
        if (adj.getX() < 0 || adj.getX() >= Screen::MAX_X || adj.getY() < 0 || adj.getY() >= Screen::MAX_Y) continue;
        wchar_t ch = currentScreen.getCharAt(adj);
        if (Glyph::isSpecialDoor(ch)) {
            auto* door = game.findSpecialDoorAt(currentRoomIdx, adj);
            if (door && door->useKey(Key(getCarried()))) {
                setCarried(NO_CARRIED_ITEM);
                break;
            }
        }
    }
}

void Player::handleTeleportation(Screen& currentScreen, Game& game) {
    SpecialDoor* teleportDoor = game.findSpecialDoorAt(currentRoomIdx, position);
    if (teleportDoor && teleportDoor->isOpen() && teleportDoor->getTargetRoomIdx() >= 0) {
        currentScreen.refreshCell(position);
        currentRoomIdx = teleportDoor->getTargetRoomIdx();
        position = teleportDoor->getTargetPosition();
    }
}