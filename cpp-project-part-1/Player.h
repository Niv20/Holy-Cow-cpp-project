#pragma once
#include "Point.h"
#include "Screen.h"

class Player {
    static constexpr int NUM_KEYS = 6;
    Point position;
    char keys[NUM_KEYS];
    char symbol;
    int currentRoomIdx; // Remembers which room this player is in
    
    // TODO:!!!!!!!!!!!!!!!!!!
	 bool hasBomb = false;
     bool hasTorch = false;
	 char keyIcon = ' ';

public:
    Player(Point startPos, const char* keySet, char sym, int startRoom);

    void draw() const;

    void erase(Screen& currentScreen) const;

    // Logic for moving (checking walls, items)
    void move(Screen& currentScreen);

    // Sets direction based on input
    void handleKey(char key);

	bool hasKey() const { return keyIcon != ' '; }

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