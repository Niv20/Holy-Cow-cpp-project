#include "Legend.h"
#include <string>
#include <vector>
#include "Screen.h"
#include "Point.h"

void Legend::ensureRooms(size_t count) { 
    if (roomLegendPos.size() < count) 
        roomLegendPos.resize(count, Point{ -1, -1 }); 
}

void Legend::locateLegendForRoom(int roomIdx, const Screen& s) {

    if (roomIdx < 0)
        return;

    ensureRooms(static_cast<size_t>(roomIdx + 1));

    for (int y = 0; y < Screen::MAX_Y; ++y) {
        for (int x = 0; x < Screen::MAX_X; ++x) {
            Point p{ x, y }; 
            if (s.getCharAt(p) == 'L') {
                roomLegendPos[roomIdx] = p; 
                return;
            }
        }
    }
    roomLegendPos[roomIdx] = Point{ -1, -1 }; // not found
}

// Explicitly draw the anchor 'L' on screen for the given room
void Legend::drawAnchor(int roomIdx) const {
    if (roomIdx < 0 || roomIdx >= (int)roomLegendPos.size()) 
        return;
    Point tl = roomLegendPos[roomIdx]; 
    if (tl.x < 0 || tl.y < 0) 
        return;
    tl.draw('L');
}

// Written by AI
void Legend::drawLegend(int roomIdx, int lives, int points, char p1Inv, char p2Inv) {

    ensureRooms(static_cast<size_t>(roomIdx + 1));
    Point tl = roomLegendPos[roomIdx];

    if (tl.x < 0 || tl.y < 0)
        return;

    // anchor is top-left
    Point origin{ tl.x, tl.y };

    char line1[32];
    char line2[32];
    char line3[32];

    snprintf(line1, sizeof(line1), "live: %d", lives);
    snprintf(line2, sizeof(line2), "Pts: %d", points);

    snprintf(line3, sizeof(line3), "Inv: %c=[%c] %c=[%c]", 
             '\xF6', p1Inv == ' ' ? ' ' : p1Inv, 
             '\xFC', p2Inv == ' ' ? ' ' : p2Inv);

    auto pad16 = [](std::string& s) {
        if (s.size() < 16)
            s.append(16 - s.size(), ' ');
        else if (s.size() > 16)
            s.resize(16);
    };

    std::string l1(line1), l2(line2), l3(line3); 
    pad16(l1); 
    pad16(l2); 
    pad16(l3);

    auto putLine = [&](int dy, const std::string& line) {
        for (int i = 0; i < 16; ++i) {
            Point p{ origin.x + i, origin.y + dy };
            if (p.x >= 0 && p.x < Screen::MAX_X && p.y >= 0 && p.y < Screen::MAX_Y) {
                p.draw(line[i]);
            }
        }
    };

    putLine(0, l1);
    putLine(1, l2);
    putLine(2, l3);
}

// Static method to scan all legends in the world
void Legend::scanAllLegends(std::vector<Screen>& world, Legend& legend) {
    legend.ensureRooms(world.size());
    for (int room = 0; room < (int)world.size(); ++room) {
        legend.locateLegendForRoom(room, world[room]);
    }
}