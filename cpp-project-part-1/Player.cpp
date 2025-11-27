#include "Player.h"
#include <cctype>
#include "Screen.h"
#include "Tiles.h"

Player::Player(Point startPos, const char* keySet, char sym, int startRoom)
    : position(startPos), symbol(sym), currentRoomIdx(startRoom)
{
    for (int i = 0; i < NUM_KEYS; i++)
         keys[i] = keySet[i];
}

void Player::draw() const {
    position.draw(symbol);
}

void Player::erase(Screen& currentScreen) const {
    currentScreen.erase(position);
}

void Player::move(Screen& currentScreen) {

    // Calculate next position
    Point originalPos = position;
    position.move();

    // If actually moved, restore the floor at the previous position to avoid flicker
    if (originalPos.x != position.x || originalPos.y != position.y) {
        char charUnderPlayer = currentScreen.getCharAt(originalPos);
        if (Tiles::isRoomTransition(charUnderPlayer) || Tiles::isRiddle(charUnderPlayer)) {
            charUnderPlayer = Tiles::Empty; // Hide door numbers and riddle markers
        }
        currentScreen.setCharAt(originalPos, charUnderPlayer);
    }

    // Check tile at new position
    char tile = currentScreen.getCharAt(position);

    // Handle known interactive tiles first
    if (Tiles::isKey(tile)) {
        if (canTakeObject()) {
            setCarried(tile);
            currentScreen.setCharAt(position, Tiles::Empty);
        }
    }
    else if (Tiles::isDoor(tile)) {
        if (getCarried() == std::tolower(static_cast<unsigned char>(tile))) {
            currentScreen.setCharAt(position, Tiles::Empty); // unlock
            setCarried(' ');
        } else {
            // blocked by locked door
            position = originalPos;
            position.setDirection(4);
        }
    }
    else if (Tiles::isBomb(tile)) {
        if (canTakeObject()) {
            setCarried(Tiles::Bomb);
            currentScreen.setCharAt(position, Tiles::Empty);
        }
    }
    else if (Tiles::isTorch(tile)) {
        if (canTakeObject()) {
            setCarried(Tiles::Torch);
            currentScreen.setCharAt(position, Tiles::Empty);
        }
    }
    else if (Tiles::isRiddle(tile) || Tiles::isRoomTransition(tile)) {
        // allow passing; handled by Game elsewhere
    }
    else if (Tiles::isWall(tile)) {
        // blocked by wall
        position = originalPos;
        position.setDirection(4);
    }
    else if (tile == Tiles::Empty) {
        // free space, continue
    }
    else {
        // Unknown non-empty tile: block and set STAY
        position = originalPos;
        position.setDirection(4);
    }

    // Handle action (E/O): drop or pick
    if (actionRequested) {
        char held = getCarried();
        if (held != ' ') {
            if (!isStationary()) {
                // After pressing and moving one tile, drop adjacent (not under player)
                Point p = position;
                Point candidates[4] = { { p.x + 1, p.y }, { p.x - 1, p.y }, { p.x, p.y - 1 }, { p.x, p.y + 1 } };
                for (int i = 0; i < 4; ++i) {
                    Point q = candidates[i];
                    if (q.x >= 0 && q.x < Screen::MAX_X && q.y >= 0 && q.y < Screen::MAX_Y) {
                        if (currentScreen.getCharAt(q) == Tiles::Empty) {
                            currentScreen.setCharAt(q, held);
                            setCarried(' ');
                            break;
                        }
                    }
                }
            } else {
                // Stationary: try right, left, up, then down
                Point p = position;
                Point candidates[4] = { { p.x + 1, p.y }, { p.x - 1, p.y }, { p.x, p.y - 1 }, { p.x, p.y + 1 } };
                for (int i = 0; i < 4; ++i) {
                    Point q = candidates[i];
                    if (q.x >= 0 && q.x < Screen::MAX_X && q.y >= 0 && q.y < Screen::MAX_Y) {
                        if (currentScreen.getCharAt(q) == Tiles::Empty) {
                            currentScreen.setCharAt(q, held);
                            setCarried(' ');
                            break; // place only once
                        }
                    }
                }
            }
        } else {
            // No held item: pick if standing on collectible
            char under = currentScreen.getCharAt(position);
            if ((Tiles::isKey(under) || Tiles::isBomb(under) || Tiles::isTorch(under)) && canTakeObject()) {
                setCarried(under);
                currentScreen.setCharAt(position, Tiles::Empty);
            }
        }
        actionRequested = false; // consume action
    }

    // Draw player at (possibly unchanged) position
    draw();
}

void Player::handleKey(char key) {
    for (int i = 0; i < NUM_KEYS; i++) {
        if (std::tolower(key) == std::tolower(keys[i])) {
            // חמשת הכיוונים: למעלה, ימינה, למטה, שמאלה, לא זז
            position.setDirection(i);
            if (i == 5) {
                actionRequested = true; // E/O action
            }
            return;
        }
    }
}

void Player::stop() {
    position.setDirection(4); // STAY
}