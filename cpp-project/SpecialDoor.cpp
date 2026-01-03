#include "SpecialDoor.h"
#include "Game.h"
#include "Switch.h"
#include "Screen.h"
#include "Glyph.h"
#include "FileParser.h"
#include <algorithm>
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
        
        for (const auto& doorMeta : meta.getDoors()) {
            SpecialDoor door((int)room, doorMeta.getPosition());
            
            // Add required keys
            for (char key : doorMeta.getRequiredKeys()) {
                door.addRequiredKey(Key(key));
            }
            
            // Add switch requirements
            for (const auto& switchReq : doorMeta.getSwitchRequirements()) {
                door.addRequiredSwitch(SwitchRequirement(switchReq.first, switchReq.second));
            }
            
            // Set teleport target if specified
            door.setTargetRoomIdx(doorMeta.getTargetRoom());
            door.setTargetPosition(doorMeta.getTargetPosition());
            
            // Adjust position to actual door glyph
            adjustDoorPosition(&door, world);
            
            // Add to screen's door list
            world[room].getDataMutable().doors.push_back(door);
        }
    }
}

void SpecialDoor::scanAndPopulate(std::vector<Screen>& world) {
    // Clear all screen doors first
    for (size_t room = 0; room < world.size(); ++room) {
        world[room].getDataMutable().doors.clear();
    }
    
    // Load doors from screen metadata
    loadDoorsFromMetadata(world);
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
