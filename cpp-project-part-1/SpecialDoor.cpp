#include "SpecialDoor.h"
#include "Game.h"
#include "Switch.h"
#include "Screen.h"
#include "Glyph.h"
#include "SpecialDoorsData.h"
#include <algorithm>
#include <sstream>
#include <climits>

// This file written by AI

bool SpecialDoor::areConditionsMet(Game& game) {
    if (isOpen) return true;
    if (keysInserted.size() != requiredKeys.size()) return false;
    for (const auto& rk : requiredKeys) {
        auto it = std::find_if(keysInserted.begin(), keysInserted.end(), [&](const Key& k){ return k.equals(rk); });
        if (it == keysInserted.end()) return false;
    }
    for (const auto& req : requiredSwitches) {
        SwitchData* sw = game.findSwitchAt(roomIdx, req.pos);
        if (!sw) continue;
        if (sw->isOn != req.requiredState) return false;
    }
    isOpen = true; return true;
}

bool SpecialDoor::useKey(const Key& key) {
    if (isOpen || !key.valid()) return false;
    auto needed = std::find_if(requiredKeys.begin(), requiredKeys.end(), [&](const Key& k){ return k.equals(key); });
    if (needed == requiredKeys.end()) return false;
    auto already = std::find_if(keysInserted.begin(), keysInserted.end(), [&](const Key& k){ return k.equals(key); });
    if (already != keysInserted.end()) return false;
    keysInserted.push_back(key); return true;
}

void SpecialDoor::scanAndPopulate(std::vector<Screen>& world) {
    // Clear all screen doors first
    for (size_t room = 0; room < world.size(); ++room) {
        world[room].getDataMutable().doors.clear();
    }
    
    std::vector<std::string> lines;
    std::string config(SPECIAL_DOORS_CONFIG);
    std::istringstream iss(config);
    std::string line;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }
    
    if (lines.empty()) return;

    SpecialDoor* currentDoor = nullptr;

    auto adjustDoorPosition = [&](SpecialDoor* d) {
        if (!d) return;
        Screen& s = world[d->roomIdx];
        Point cfg = d->position;
        bool inBounds = (cfg.x >= 0 && cfg.x < Screen::MAX_X && cfg.y >= 0 && cfg.y < Screen::MAX_Y);
        bool atDoor = inBounds && s.getCharAt(cfg) == Glyph::SpecialDoor;
        if (atDoor) return;
        int bestDist = INT_MAX; Point bestPos = cfg; bool found = false;
        for (int y=0;y<Screen::MAX_Y;++y) for (int x=0;x<Screen::MAX_X;++x) {
            Point p{x,y}; if (s.getCharAt(p)==Glyph::SpecialDoor) {
                int dist = abs(x-cfg.x)+abs(y-cfg.y); if (dist<bestDist){bestDist=dist;bestPos=p;found=true;}
            }
        }
        if (found) d->position=bestPos; else d->isOpen=true;
    };

    for (auto& raw : lines) {
        if (raw.empty() || raw[0]=='#') continue;
        std::stringstream ss(raw); char type; ss >> type;
        if (type=='D') {
            if (currentDoor) { 
                adjustDoorPosition(currentDoor); 
                world[currentDoor->roomIdx].getDataMutable().doors.push_back(*currentDoor); 
                delete currentDoor; 
            }
            int room,x,y; ss>>room>>x>>y; currentDoor = new SpecialDoor(room, Point(x,y));
        } else if (type=='K' && currentDoor) {
            char key; while (ss>>key) currentDoor->requiredKeys.push_back(Key(key));
        } else if (type=='S' && currentDoor) {
            int sx,sy,state; ss>>sx>>sy>>state; currentDoor->requiredSwitches.push_back({Point(sx,sy),(bool)state});
        } else if (type=='T' && currentDoor) {
            // Teleport destination: T targetRoom targetX targetY
            int tRoom, tX, tY; ss>>tRoom>>tX>>tY;
            currentDoor->targetRoomIdx = tRoom;
            currentDoor->targetPosition = Point(tX, tY);
        } else if (raw.rfind("---",0)==0) {
            if (currentDoor) { 
                adjustDoorPosition(currentDoor); 
                world[currentDoor->roomIdx].getDataMutable().doors.push_back(*currentDoor); 
                delete currentDoor; 
                currentDoor=nullptr; 
            }
        }
    }
    if (currentDoor) { 
        adjustDoorPosition(currentDoor); 
        world[currentDoor->roomIdx].getDataMutable().doors.push_back(*currentDoor); 
        delete currentDoor; 
    }
}

void SpecialDoor::updateAll(Game& game) {
    int visibleRoomIdx = game.getVisibleRoomIdx();
    int worldSize = game.getWorldSize();
    
    for (int room = 0; room < worldSize; ++room) {
        Screen& s = game.getScreen(room);
        auto& doors = s.getDataMutable().doors;
        
        for (auto& door : doors) {
            if (!door.isOpen && door.areConditionsMet(game)) {
                // Only remove the door glyph if it's NOT a teleport door
                // Teleport doors stay visible so players can use them multiple times
                if (door.targetRoomIdx < 0) {
                    Screen& doorScreen = game.getScreen(door.roomIdx);
                    if (doorScreen.getCharAt(door.position) == Glyph::SpecialDoor) {
                        doorScreen.setCharAt(door.position, Glyph::Empty);
                        if (door.roomIdx == visibleRoomIdx) {
                            doorScreen.refreshCell(door.position);
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
        if (door.position.x == p.x && door.position.y == p.y) {
            return &door;
        }
    }
    return nullptr;
}
