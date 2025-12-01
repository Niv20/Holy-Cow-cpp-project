#pragma once
#include "Point.h"
#include "Key.h"
#include <vector>
#include <string>

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
    bool isOpen = false;

    SpecialDoor(int room, Point p) : roomIdx(room), position(p) {}

    bool areConditionsMet(class Game& game); // check if all conditions satisfied
    bool useKey(const Key& key);                  // attempt to insert a key
};
