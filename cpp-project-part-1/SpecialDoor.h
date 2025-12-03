#pragma once
#include "Point.h"
#include "Key.h"
#include <vector>
#include <string>

// Forward declarations
class Game;
class Screen;

// Defines a requirement for a switch state for a special door
struct SwitchRequirement {
    Point pos;
    bool requiredState; // true for '1' (ON), false for '0' (OFF)
};

// Represents a special door with opening conditions (keys + specific switches)
class SpecialDoor {
public:
    int roomIdx;
    Point position;
    std::vector<Key> requiredKeys;              // keys required (unique)
    std::vector<SwitchRequirement> requiredSwitches; // explicit point-based switch requirements

    // State tracking
    std::vector<Key> keysInserted; // keys already provided
    mutable bool isOpen = false;

    SpecialDoor(int room, Point p) : roomIdx(room), position(p) {}

    bool areConditionsMet(Game& game); // check if all conditions satisfied
    bool useKey(const Key& key);                  // attempt to insert a key
    
    // Static methods for managing all special doors
    static void scanAndPopulate(std::vector<Screen>& world);
    static void updateAll(Game& game);
    
    // Static: Find special door at position in a screen
    static SpecialDoor* findAt(Screen& screen, const Point& p);
};
