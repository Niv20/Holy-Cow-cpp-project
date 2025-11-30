#include "Player.h"
#include <cctype>
#include "Screen.h"
#include "Glyph.h" // was Tiles.h
#include <windows.h>

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

void Player::move(Screen& currentScreen) {
    Point originalPos = position; position.move();
    wchar_t tile = currentScreen.getCharAt(position);
    bool blocked = false;

    if (Glyph::isKey(tile)) {
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
        } else blocked = true;
    } else if (Glyph::isBomb(tile)) {
        if (canTakeObject()) { setCarried('@'); currentScreen.setCharAt(position, Glyph::Empty); currentScreen.refreshCell(position); }
    } else if (Glyph::isTorch(tile)) {
        if (canTakeObject()) { setCarried('!'); currentScreen.setCharAt(position, Glyph::Empty); currentScreen.refreshCell(position); }
    } else if (Glyph::isWall(tile)) blocked = true;
    else if (Glyph::isRiddle(tile) || tile == Glyph::Empty) {
        // allowed
    } else blocked = true;

    if (blocked) { position = originalPos; position.setDirection(4); return; }

    if (originalPos.x != position.x || originalPos.y != position.y) currentScreen.refreshCell(originalPos);

    if (actionRequested) {
        char held = getCarried();
        if (held != ' ') {
            bool dropped = false;
            Point p = position;
            // Drop behind movement direction
            if (position.diff_x != 0 || position.diff_y != 0) {
                Point drop = p;
                if (position.diff_x == 1) drop.x = p.x - 1; // moving right -> drop left
                else if (position.diff_x == -1) drop.x = p.x + 1; // moving left -> drop right
                else if (position.diff_y == 1) drop.y = p.y - 1; // moving down -> drop up
                else if (position.diff_y == -1) drop.y = p.y + 1; // moving up -> drop down
                if (drop.x>=0 && drop.x<Screen::MAX_X && drop.y>=0 && drop.y<Screen::MAX_Y) {
                    if (currentScreen.getCharAt(drop) == Glyph::Empty) {
                        currentScreen.setCharAt(drop,(wchar_t)held);
                        currentScreen.refreshCell(drop);
                        setCarried(' '); dropped = true;
                    }
                }
            }
            // If stationary or behind cell blocked: follow preference Right, Left, Up, Down
            if (!dropped) {
                Point candidates[4] = { {p.x+1,p.y},{p.x-1,p.y},{p.x,p.y-1},{p.x,p.y+1} };
                for (auto& q : candidates) {
                    if (q.x>=0 && q.x<Screen::MAX_X && q.y>=0 && q.y<Screen::MAX_Y && currentScreen.getCharAt(q) == Glyph::Empty) {
                        currentScreen.setCharAt(q,(wchar_t)held);
                        currentScreen.refreshCell(q);
                        setCarried(' '); dropped = true; break;
                    }
                }
            }
        } else {
            // Pick up again if standing on item while stationary action
            wchar_t under = currentScreen.getCharAt(position);
            if ((Glyph::isKey(under) || Glyph::isBomb(under) || Glyph::isTorch(under)) && canTakeObject()) {
                setCarried((char)under); currentScreen.setCharAt(position, Glyph::Empty); currentScreen.refreshCell(position);
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