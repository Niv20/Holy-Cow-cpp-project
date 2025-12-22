#pragma once
#include "Point.h"

// Forward declaration
class Screen;

// Represents a switch that can be toggled ON (1) or OFF (0) by stepping on it.
// When stepped on, the switch toggles state and the player is pushed back one step.
struct SwitchData {
    int roomIdx;
    Point pos;
    bool isOn; // true = '1', false = '0'
    
    SwitchData(int room, Point p, bool initialState = false)
        : roomIdx(room), pos(p), isOn(initialState) {}
    
    // Toggle the switch state
    void toggle() {
        isOn = !isOn;
    }
    
    // Get current display character
    wchar_t getDisplayChar() const {
        return isOn ? L'1' : L'0';
    }
    
    // Static: Find switch at position in a screen
    static SwitchData* findAt(Screen& screen, const Point& p);
};
