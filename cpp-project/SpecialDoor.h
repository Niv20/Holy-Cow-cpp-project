#pragma once
#include "Point.h"
#include "Key.h"
#include <vector>
#include <string>

// Forward declarations
class Game;
class Screen;

// Defines a requirement for a switch state for a special door
class SwitchRequirement {
private:
    Point pos_;
    bool requiredState_; // true for '1' (ON), false for '0' (OFF)
    
public:
    SwitchRequirement() : pos_(0, 0), requiredState_(false) {}
    SwitchRequirement(Point p, bool state) : pos_(p), requiredState_(state) {}
    
    Point getPos() const { return pos_; }
    bool getRequiredState() const { return requiredState_; }
};

// Represents a special door with opening conditions (keys + specific switches)
class SpecialDoor {
private:
    int roomIdx_;
    Point position_;
    std::vector<Key> requiredKeys_;              // keys required (unique)
    std::vector<SwitchRequirement> requiredSwitches_; // explicit point-based switch requirements

    // Optional teleport destination (if targetRoomIdx >= 0, door teleports player)
    int targetRoomIdx_ = -1;  // -1 means no teleport, just opens
    Point targetPosition_;    // destination position in target room

    // State tracking
    std::vector<Key> keysInserted_; // keys already provided
    bool isOpen_ = false;

public:
    SpecialDoor(int room, Point p) : roomIdx_(room), position_(p), targetRoomIdx_(-1), targetPosition_(0, 0) {}

    // Getters
    int getRoomIdx() const { return roomIdx_; }
    Point getPosition() const { return position_; }
    const std::vector<Key>& getRequiredKeys() const { return requiredKeys_; }
    const std::vector<SwitchRequirement>& getRequiredSwitches() const { return requiredSwitches_; }
    int getTargetRoomIdx() const { return targetRoomIdx_; }
    Point getTargetPosition() const { return targetPosition_; }
    const std::vector<Key>& getKeysInserted() const { return keysInserted_; }
    bool isOpen() const { return isOpen_; }
    
    // Setters
    void setPosition(Point p) { position_ = p; }
    void setTargetRoomIdx(int idx) { targetRoomIdx_ = idx; }
    void setTargetPosition(Point p) { targetPosition_ = p; }
    void setOpen(bool open) { isOpen_ = open; }
    
    // Modifiers for collections
    void addRequiredKey(const Key& key) { requiredKeys_.push_back(key); }
    void addRequiredSwitch(const SwitchRequirement& sw) { requiredSwitches_.push_back(sw); }
    void addInsertedKey(const Key& key) { keysInserted_.push_back(key); }

    bool areConditionsMet(Game& game); // check if all conditions satisfied
    bool useKey(const Key& key);                  // attempt to insert a key
    
    // Static methods for managing all special doors
    static void scanAndPopulate(std::vector<Screen>& world);
    static void updateAll(Game& game);
    
    // Static: Find special door at position in a screen
    static SpecialDoor* findAt(Screen& screen, const Point& p);
};
