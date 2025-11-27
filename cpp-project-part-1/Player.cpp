#include "Player.h"
#include <cctype>
#include "Screen.h"
#include "Tiles.h"

Player::Player(Point startPos, const char* keySet, char sym, int startRoom)
    : position(startPos), symbol(sym), currentRoomIdx(startRoom)
{ for (int i = 0; i < NUM_KEYS; i++) keys[i] = keySet[i]; }

void Player::draw() const { position.draw(symbol); }
void Player::erase(Screen& currentScreen) const { currentScreen.erase(position); }

void Player::move(Screen& currentScreen) {
    Point originalPos = position; position.move();
    wchar_t tile = currentScreen.getCharAt(position);

    if (Tiles::isKey(tile)) {
        if (canTakeObject()) { setCarried((char)tile); currentScreen.setCharAt(position, Tiles::Empty); }
    } else if (Tiles::isDoor(tile)) {
        if (getCarried() == std::tolower((unsigned char)tile)) { currentScreen.setCharAt(position, Tiles::Empty); setCarried(' '); }
        else { position = originalPos; position.setDirection(4); }
    } else if (Tiles::isBomb(tile)) {
        if (canTakeObject()) { setCarried('@'); currentScreen.setCharAt(position, Tiles::Empty); }
    } else if (Tiles::isTorch(tile)) {
        if (canTakeObject()) { setCarried('!'); currentScreen.setCharAt(position, Tiles::Empty); }
    } else if (Tiles::isWall(tile)) { position = originalPos; position.setDirection(4); }
    else if (tile == Tiles::Empty) { }
    else if (Tiles::isRiddle(tile) || Tiles::isRoomTransition(tile)) { }
    else { position = originalPos; position.setDirection(4); }

    if (actionRequested) {
        char held = getCarried();
        if (held != ' ') {
            Point p = position; Point candidates[4] = { {p.x+1,p.y},{p.x-1,p.y},{p.x,p.y-1},{p.x,p.y+1} };
            for (auto& q : candidates) {
                if (q.x>=0 && q.x<Screen::MAX_X && q.y>=0 && q.y<Screen::MAX_Y) {
                    if (currentScreen.getCharAt(q) == Tiles::Empty) { currentScreen.setCharAt(q, (wchar_t)held); setCarried(' '); break; }
                }
            }
        } else {
            wchar_t under = currentScreen.getCharAt(position);
            if ((Tiles::isKey(under) || Tiles::isBomb(under) || Tiles::isTorch(under)) && canTakeObject()) {
                setCarried((char)under); currentScreen.setCharAt(position, Tiles::Empty);
            }
        }
        actionRequested = false;
    }
    draw();
}

void Player::handleKey(char key) {
    for (int i = 0; i < NUM_KEYS; i++) {
        if (std::tolower(key) == std::tolower(keys[i])) { position.setDirection(i); if (i==5) actionRequested = true; return; }
    }
}
void Player::stop() { position.setDirection(4); }