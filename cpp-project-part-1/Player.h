#pragma once
#include "Point.h"
#include "Screen.h"

class Player {
    static constexpr int NUM_KEYS = 6;
    Point position;
    char keys[NUM_KEYS];
    char symbol;
    int currentRoomIdx;
    
    // Single carried item character: ' ' none, 'a'..'z' key, '!' torch, '@' bomb
    char carried = ' ';

    // Action handling
    bool actionRequested = false; // toggled by last key (E/O)

public:
    Player(Point startPos, const char* keySet, char sym, int startRoom);

    void draw() const;

    void erase(Screen& currentScreen) const;

    // Logic for moving (checking walls, items)
    void move(Screen& currentScreen);

    // Sets direction based on input
    void handleKey(char key);

    // Reset movement to STAY
    void stop();

    // Is player stationary (no movement direction)?
    bool isStationary() const {
        return position.diff_x == 0 && position.diff_y == 0; 
    }

    bool canTakeObject() const {
        return carried == ' '; 
    }

    // Getters / Setters
    int getRoomIdx() const { return currentRoomIdx; }
    void setRoomIdx(int idx) { currentRoomIdx = idx; }
    Point getPosition() const { return position; }
    void setPosition(Point p) { position = p; }

    char getCarried() const { return carried; }
    void setCarried(char ch) { carried = ch; }
};