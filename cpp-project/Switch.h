#pragma once
#include "Point.h"

// Forward declaration
class Screen;

// Represents a switch that can be toggled ON (1) or OFF (0) by stepping on it.
// When stepped on, the switch toggles state and the player is pushed back one step.
class SwitchData {
private:
    int roomIdx_;
    Point pos_;
    bool isOn_; // true = '1', false = '0'
    
public:
    SwitchData(int room, Point p, bool initialState = false)
        : roomIdx_(room), pos_(p), isOn_(initialState) {}
    
    // Getters
    int getRoomIdx() const { return roomIdx_; }
    Point getPos() const { return pos_; }
    bool isOn() const { return isOn_; }
    
    // Toggle the switch state
    void toggle() {
        isOn_ = !isOn_;
    }
    
    // Get current display character
    wchar_t getDisplayChar() const {
        return isOn_ ? L'1' : L'0';
    }
    
    // Static: Find switch at position in a screen
    static SwitchData* findAt(Screen& screen, const Point& p);
};
