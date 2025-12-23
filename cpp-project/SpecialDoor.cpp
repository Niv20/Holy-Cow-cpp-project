#include "SpecialDoor.h"
#include "Game.h"
#include "Switch.h"
#include "Screen.h"
#include "Glyph.h"
#include "SpecialDoorsData.h"
#include "FileParser.h"
#include <algorithm>
#include <sstream>
#include <climits>

// This file written by AI

bool SpecialDoor::areConditionsMet(Game& game) {
    if (isOpen_) return true;
    if (keysInserted_.size() != requiredKeys_.size()) return false;
    for (const auto& rk : requiredKeys_) {
        auto it = std::find_if(keysInserted_.begin(), keysInserted_.end(), [&](const Key& k){ return k.equals(rk); });
        if (it == keysInserted_.end()) return false;
    }
    for (const auto& req : requiredSwitches_) {
        SwitchData* sw = game.findSwitchAt(roomIdx_, req.getPos());
        if (!sw) continue;
        if (sw->isOn() != req.getRequiredState()) return false;
    }
    isOpen_ = true; return true;
}

bool SpecialDoor::useKey(const Key& key) {
    if (isOpen_ || !key.valid()) return false;
    auto needed = std::find_if(requiredKeys_.begin(), requiredKeys_.end(), [&](const Key& k){ return k.equals(key); });
    if (needed == requiredKeys_.end()) return false;
    auto already = std::find_if(keysInserted_.begin(), keysInserted_.end(), [&](const Key& k){ return k.equals(key); });
    if (already != keysInserted_.end()) return false;
    keysInserted_.push_back(key); return true;
}

// Helper: Adjust door position to nearest actual door glyph
static void adjustDoorPosition(SpecialDoor* d, std::vector<Screen>& world) {
if (!d || d->getRoomIdx() < 0 || d->getRoomIdx() >= (int)world.size()) return;
    
Screen& s = world[d->getRoomIdx()];
Point cfg = d->getPosition();
    bool inBounds = (cfg.getX() >= 0 && cfg.getX() < Screen::MAX_X && cfg.getY() >= 0 && cfg.getY() < Screen::MAX_Y);
    bool atDoor = inBounds && s.getCharAt(cfg) == Glyph::SpecialDoor;
    if (atDoor) return;
    
    int bestDist = INT_MAX; 
    Point bestPos = cfg; 
    bool found = false;
    
    for (int y = 0; y < Screen::MAX_Y; ++y) {
        for (int x = 0; x < Screen::MAX_X; ++x) {
            Point p(x, y); 
            if (s.getCharAt(p) == Glyph::SpecialDoor) {
                int dist = abs(x - cfg.getX()) + abs(y - cfg.getY()); 
                if (dist < bestDist) {
                    bestDist = dist;
                    bestPos = p;
                    found = true;
                }
            }
        }
    }
    
    
    if (found) {
        d->setPosition(bestPos);
    } else {
        // No door glyph found, mark as open (non-blocking)
        d->setOpen(true);
    }
}

// Load doors from screen metadata
static void loadDoorsFromMetadata(std::vector<Screen>& world) {
    for (size_t room = 0; room < world.size(); ++room) {
        const ScreenMetadata& meta = world[room].getMetadata();
        
        for (const auto& doorMeta : meta.doors) {
            SpecialDoor door((int)room, doorMeta.position);
            
            // Add required keys
            for (char key : doorMeta.requiredKeys) {
                door.addRequiredKey(Key(key));
            }
            
            // Add switch requirements
            for (const auto& [pos, state] : doorMeta.switchRequirements) {
                door.addRequiredSwitch(SwitchRequirement(pos, state));
            }
            
            // Set teleport target if specified
            door.setTargetRoomIdx(doorMeta.targetRoom);
            door.setTargetPosition(doorMeta.targetPosition);
            
            // Adjust position to actual door glyph
            adjustDoorPosition(&door, world);
            
            // Add to screen's door list
            world[room].getDataMutable().doors.push_back(door);
        }
    }
}

// Load doors from legacy fallback config (for screens without metadata)
static void loadDoorsFromLegacyConfig(std::vector<Screen>& world) {
    std::vector<std::string> lines;
    std::string config(SPECIAL_DOORS_CONFIG);
    std::istringstream iss(config);
    std::string line;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }
    
    if (lines.empty()) return;

    SpecialDoor* currentDoor = nullptr;

    for (auto& raw : lines) {
        if (raw.empty() || raw[0] == '#') continue;
        std::stringstream ss(raw); 
        char type; 
        ss >> type;
        
        if (type == 'D') {
            if (currentDoor) { 
                adjustDoorPosition(currentDoor, world); 
                if (currentDoor->getRoomIdx() >= 0 && currentDoor->getRoomIdx() < (int)world.size()) {
                    // Only add if no door already exists from metadata
                    auto& existingDoors = world[currentDoor->getRoomIdx()].getDataMutable().doors;
                    bool alreadyExists = false;
                    for (const auto& d : existingDoors) {
                        Point dPos = d.getPosition();
                        Point curPos = currentDoor->getPosition();
                        if (dPos.getX() == curPos.getX() && dPos.getY() == curPos.getY()) {
                            alreadyExists = true;
                            break;
                        }
                    }
                    if (!alreadyExists) {
                        existingDoors.push_back(*currentDoor);
                    }
                }
                delete currentDoor; 
            }
            int room, x, y; 
            ss >> room >> x >> y; 
            currentDoor = new SpecialDoor(room, Point(x, y));
        } 
        else if (type == 'K' && currentDoor) {
            char key; 
            while (ss >> key) {
            currentDoor->addRequiredKey(Key(key));
            }
        } 
        else if (type == 'S' && currentDoor) {
            int sx, sy, state; 
            ss >> sx >> sy >> state; 
            currentDoor->addRequiredSwitch(SwitchRequirement(Point(sx, sy), (bool)state));
        } 
        else if (type == 'T' && currentDoor) {
            int tRoom, tX, tY; 
            ss >> tRoom >> tX >> tY;
            currentDoor->setTargetRoomIdx(tRoom);
            currentDoor->setTargetPosition(Point(tX, tY));
        } 
        else if (raw.rfind("---", 0) == 0) {
            if (currentDoor) { 
                adjustDoorPosition(currentDoor, world); 
                if (currentDoor->getRoomIdx() >= 0 && currentDoor->getRoomIdx() < (int)world.size()) {
                    auto& existingDoors = world[currentDoor->getRoomIdx()].getDataMutable().doors;
                    bool alreadyExists = false;
                    for (const auto& d : existingDoors) {
                        Point dPos = d.getPosition();
                        Point curPos = currentDoor->getPosition();
                        if (dPos.getX() == curPos.getX() && dPos.getY() == curPos.getY()) {
                            alreadyExists = true;
                            break;
                        }
                    }
                    if (!alreadyExists) {
                        existingDoors.push_back(*currentDoor);
                    }
                }
                delete currentDoor; 
                currentDoor = nullptr; 
            }
        }
    }
    
    
    if (currentDoor) { 
        adjustDoorPosition(currentDoor, world); 
        if (currentDoor->getRoomIdx() >= 0 && currentDoor->getRoomIdx() < (int)world.size()) {
            auto& existingDoors = world[currentDoor->getRoomIdx()].getDataMutable().doors;
            bool alreadyExists = false;
            for (const auto& d : existingDoors) {
                Point dPos = d.getPosition();
                Point curPos = currentDoor->getPosition();
                if (dPos.getX() == curPos.getX() && dPos.getY() == curPos.getY()) {
                    alreadyExists = true;
                    break;
                }
            }
            if (!alreadyExists) {
                existingDoors.push_back(*currentDoor);
            }
        }
        delete currentDoor; 
    }
}

void SpecialDoor::scanAndPopulate(std::vector<Screen>& world) {
    // Clear all screen doors first
    for (size_t room = 0; room < world.size(); ++room) {
        world[room].getDataMutable().doors.clear();
    }
    
    // First, load doors from screen metadata (preferred)
    loadDoorsFromMetadata(world);
    
    // Then, load from legacy config for any doors not defined in metadata
    loadDoorsFromLegacyConfig(world);
}

void SpecialDoor::updateAll(Game& game) {
    int visibleRoomIdx = game.getVisibleRoomIdx();
    int worldSize = game.getWorldSize();
    
    for (int room = 0; room < worldSize; ++room) {
        Screen& s = game.getScreen(room);
        auto& doors = s.getDataMutable().doors;
        
        for (auto& door : doors) {
            if (!door.isOpen() && door.areConditionsMet(game)) {
                // Only remove the door glyph if it's NOT a teleport door
                // Teleport doors stay visible so players can use them multiple times
                if (door.getTargetRoomIdx() < 0) {
                    Screen& doorScreen = game.getScreen(door.getRoomIdx());
                    if (doorScreen.getCharAt(door.getPosition()) == Glyph::SpecialDoor) {
                        doorScreen.setCharAt(door.getPosition(), Glyph::Empty);
                        if (door.getRoomIdx() == visibleRoomIdx) {
                            doorScreen.refreshCell(door.getPosition());
                        }
                    }
                }
            }
        }
    }
}

// Static: Find special door at position
SpecialDoor* SpecialDoor::findAt(Screen& screen, const Point& p) {
    auto& dataDoors = screen.getDataMutable().doors;
    for (auto& door : dataDoors) {
        Point doorPos = door.getPosition();
        if (doorPos.getX() == p.getX() && doorPos.getY() == p.getY()) {
            return &door;
        }
    }
    return nullptr;
}
