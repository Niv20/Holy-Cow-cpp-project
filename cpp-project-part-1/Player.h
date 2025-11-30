#pragma once
#include "Point.h"
#include "Screen.h"

class Player {
    static constexpr int NUM_KEYS = 6;
    Point position;
    char keys[NUM_KEYS];
    wchar_t symbol;
    int currentRoomIdx;
    char carried = ' ';
    bool actionRequested = false;

    // Spring state
    class SpringData* currentSpring = nullptr; // which spring we're on
    int entryIndex = -1; // which cell index we entered at
    int compressedCount = 0; // how many cells compressed so far
    
    // Boost state after release
    int springBoostTicksLeft = 0;
    int springBoostSpeed = 0;
    int boostDirX = 0;
    int boostDirY = 0;

public:
    Player(Point startPos, const char* keySet, wchar_t sym, int startRoom);
    void draw() const;
    void erase(Screen& currentScreen) const;
    void move(Screen& currentScreen, class Game& game);
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

    int getForce() const { return (springBoostTicksLeft > 0) ? springBoostSpeed : 1; }
    bool isSpringBoostActive() const { return springBoostTicksLeft > 0; }
    void inheritSpringLaunch(int speed, int ticks, int dirX, int dirY);
    
private:
    void releaseSpring(Screen& currentScreen, class Game& game);
};