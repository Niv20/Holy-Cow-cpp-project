#pragma once
#include "Point.h"
#include "Screen.h"

class Player {
    static constexpr int NUM_KEYS = 6;
    Point position;
    char keys[NUM_KEYS];
    wchar_t symbol; // wide symbol
    int currentRoomIdx;
    char carried = ' '; // inventory single item
    bool actionRequested = false; // toggled by last key (E/O)
public:
    Player(Point startPos, const char* keySet, wchar_t sym, int startRoom);
    void draw() const;
    void erase(Screen& currentScreen) const;
    void move(Screen& currentScreen);
    void handleKey(char key);
    void stop();
    bool isStationary() const { return position.diff_x == 0 && position.diff_y == 0; }
    bool canTakeObject() const { return carried == ' '; }
    int getRoomIdx() const { return currentRoomIdx; }
    void setRoomIdx(int idx) { currentRoomIdx = idx; }
    Point getPosition() const { return position; }
    void setPosition(Point p) { position = p; }
    char getCarried() const { return carried; }
    void setCarried(char ch) { carried = ch; }
};