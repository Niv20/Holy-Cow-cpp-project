#pragma once
#include "Point.h"
#include "Screen.h"
#include "Key.h"

class Player {
    static constexpr int NUM_KEYS = 6;
    Point position;
    char keys[NUM_KEYS];
    wchar_t symbol;
    int currentRoomIdx;
    Key carried; // was char
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
    Key getCarriedKey() const { return carried; }
    char getCarried() const { return carried.get(); }
    void setCarried(char ch) { carried = Key(ch); }
    bool canTakeObject() const { return !carried.valid(); }
    int getRoomIdx() const { return currentRoomIdx; }
    void setRoomIdx(int idx) { currentRoomIdx = idx; }
    Point getPosition() const { return position; }
    void setPosition(Point p) { position = p; }

    int getForce() const { return (springBoostTicksLeft > 0) ? springBoostSpeed : 1; }
    bool isSpringBoostActive() const { return springBoostTicksLeft > 0; }
    void inheritSpringLaunch(int speed, int ticks, int dirX, int dirY);
    
    // Spring state accessors (for SpringLogic namespace)
    class SpringData* getCurrentSpring() const { return currentSpring; }
    void setCurrentSpring(class SpringData* spring) { currentSpring = spring; }
    int getEntryIndex() const { return entryIndex; }
    void setEntryIndex(int idx) { entryIndex = idx; }
    int getCompressedCount() const { return compressedCount; }
    void setCompressedCount(int count) { compressedCount = count; }
    
    // Boost state accessors
    int getSpringBoostTicksLeft() const { return springBoostTicksLeft; }
    int getSpringBoostSpeed() const { return springBoostSpeed; }
    int getBoostDirX() const { return boostDirX; }
    int getBoostDirY() const { return boostDirY; }
    void setBoostState(int speed, int ticks, int dirX, int dirY) {
        springBoostSpeed = speed;
        springBoostTicksLeft = ticks;
        boostDirX = dirX;
        boostDirY = dirY;
    }
    
private:
    void releaseSpring(Screen& currentScreen, class Game& game);
};