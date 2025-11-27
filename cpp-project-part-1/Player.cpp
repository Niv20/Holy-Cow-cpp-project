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

    // 1. Draw the floor under the player (erase old position)
    char charUnderPlayer = currentScreen.getCharAt(position);
    if (Tiles::isRoomTransition(charUnderPlayer) || Tiles::isRiddle(charUnderPlayer)) {
        charUnderPlayer = Tiles::Empty; // Hide door numbers and riddle markers
    }
    position.draw(charUnderPlayer);

    // 2. Calculate next position
    Point originalPos = position;
    position.move();

    // 3. Check collision
    char tile = currentScreen.getCharAt(position);

    if (Tiles::isWall(tile)) { // Wall
        position = originalPos;
    }
    else if (Tiles::isKey(tile)) { // Key
        if (canTakeObject()) {
            setKeyIcon(tile);
            currentScreen.setCharAt(position, Tiles::Empty);
        }
    }
    else if (Tiles::isDoor(tile)) { // Door
        if (getKeyIcon() == std::tolower(static_cast<unsigned char>(tile))) {
            // Unlock the door
            currentScreen.setCharAt(position, Tiles::Empty);
            setKeyIcon(' '); // Use the key
        }
        else {
            // Door is locked
            position = originalPos;
        }
    }
    // Note: '?' is not blocked - Game will handle riddle encounter

    // 3.5. Handle action (E/O): drop or pick
    if (actionRequested) {
        char held = getHeldChar();
        if (held != ' ') {
            if (!isStationary()) {
                // After pressing and moving one tile, drop at current position
                currentScreen.setCharAt(position, held);
                clearHeld();
            } else {
                // Stationary: try right, left, down, up. Only place if target is empty and within bounds.
                Point p = position;
                Point candidates[4] = { { p.x + 1, p.y }, { p.x - 1, p.y }, { p.x, p.y + 1 }, { p.x, p.y - 1 } };
                for (int i = 0; i < 4; ++i) {
                    Point q = candidates[i];
                    if (q.x >= 0 && q.x < Screen::MAX_X && q.y >= 0 && q.y < Screen::MAX_Y) {
                        if (currentScreen.getCharAt(q) == Tiles::Empty) {
                            currentScreen.setCharAt(q, held);
                            clearHeld();
                            break; // place only once
                        }
                    }
                }
            }
        } else {
            // No held item: optional future pickup by action (for now keys only if standing on one)
            char under = currentScreen.getCharAt(position);
            if (Tiles::isKey(under) && canTakeObject()) {
                setKeyIcon(under);
                currentScreen.setCharAt(position, Tiles::Empty);
            }
        }
        actionRequested = false; // consume action
    }

    // 4. Draw player at new position
    draw();
}

void Player::handleKey(char key) {
    for (int i = 0; i < NUM_KEYS; i++) {
        if (std::tolower(key) == std::tolower(keys[i])) {
            // חמשת הכיוונים: למעלה, ימינה, למטה, שמאלה, לא זז
            position.setDirection(i);
            // האחרון זה להפעיף אובייקט לבדוק לפני כן שאכן יש לי אובייקט
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