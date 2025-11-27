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

    // 4. Draw player at new position
    draw();
}

void Player::handleKey(char key) {
    for (int i = 0; i < NUM_KEYS; i++) {
        if (std::tolower(key) == std::tolower(keys[i])) {
            // חמשת הכיוונים: למעלה, ימינה, למטה, שמאלה, לא זז
            position.setDirection(i);
            // האחרון זה להפעיף אובייקט לבדוק לפני כן שאכן יש לי אובייקט
            return;
        }
    }
}

void Player::stop() {
    position.setDirection(4); // STAY
}