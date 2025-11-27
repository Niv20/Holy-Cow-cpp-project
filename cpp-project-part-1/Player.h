#pragma once
#include "Point.h"
#include "Screen.h"

class Player {
    static constexpr int NUM_KEYS = 6;
    Point position;
    char keys[NUM_KEYS];
    char symbol;
    int currentRoomIdx;
    
    bool hasBomb = false;
    bool hasTorch = false;
    char keyIcon = ' ';

    // Action handling
    bool actionRequested = false; // toggled by last key (E/O)

    // Internal helpers
    char getHeldChar() const {
        if (keyIcon != ' ') return keyIcon;
        if (hasBomb) return '@'; // Tiles::Bomb, avoid header include here
        if (hasTorch) return '!'; // Tiles::Torch
        return ' ';
    }

    void clearHeld() {
        if (keyIcon != ' ') { keyIcon = ' '; return; }
        if (hasBomb) { hasBomb = false; return; }
        if (hasTorch) { hasTorch = false; return; }
    }

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

    bool hasKey() const { 
        return keyIcon != ' '; 
    }

    bool canTakeObject() const {
        return !hasKey() && !hasBomb && !hasTorch; 
    }

    // Getters / Setters
    int getRoomIdx() const { return currentRoomIdx; }
    void setRoomIdx(int idx) { currentRoomIdx = idx; }
    Point getPosition() const { return position; }
    void setPosition(Point p) { position = p; }
    char getKeyIcon() const { return keyIcon; }
    void setKeyIcon(char icon) { keyIcon = icon; }
};