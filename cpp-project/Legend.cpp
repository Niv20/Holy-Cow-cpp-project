#include "Legend.h"
#include "ScreenBuffer.h"
#include <string>
#include <vector>
#include <windows.h>
#include "Screen.h"
#include "Point.h"

namespace {
    constexpr char LEGEND_ANCHOR_CHAR = 'L';
    constexpr int LEGEND_BUFFER_SIZE = 32;
    constexpr int LEGEND_LINE_WIDTH = 16;
    constexpr int INVALID_LEGEND_POSITION = -1;
    constexpr wchar_t PLAYER_ONE_ICON = L'\x263A';
    constexpr wchar_t PLAYER_TWO_ICON = L'\x263B';
    constexpr char EMPTY_INVENTORY_SLOT = ' ';
}

void Legend::ensureRooms(size_t count) { 
    if (roomLegendPos.size() < count) 
        roomLegendPos.resize(count, Point{ INVALID_LEGEND_POSITION, INVALID_LEGEND_POSITION }); 
}

void Legend::locateLegendForRoom(int roomIdx, const Screen& s) {

    if (roomIdx < 0)
        return;

    ensureRooms(static_cast<size_t>(roomIdx + 1));

    for (int y = 0; y < Screen::MAX_Y; ++y) {
        for (int x = 0; x < Screen::MAX_X; ++x) {
            Point p{ x, y }; 
            if (s.getCharAt(p) == LEGEND_ANCHOR_CHAR) {
                roomLegendPos[roomIdx] = p; 
                return;
            }
        }
    }
    roomLegendPos[roomIdx] = Point{ INVALID_LEGEND_POSITION, INVALID_LEGEND_POSITION }; // not found
}


// Explicitly draw the anchor 'L' on screen for the given room
void Legend::drawAnchor(int roomIdx) const {
    if (roomIdx < 0 || roomIdx >= (int)roomLegendPos.size()) 
        return;
    Point tl = roomLegendPos[roomIdx]; 
    if (tl.getX() == INVALID_LEGEND_POSITION || tl.getY() == INVALID_LEGEND_POSITION) 
        return;
    tl.draw(LEGEND_ANCHOR_CHAR);
}

// Written by AI
void Legend::drawLegend(int roomIdx, int lives, int points, char p1Inv, char p2Inv) {

    ensureRooms(static_cast<size_t>(roomIdx + 1));
    Point tl = roomLegendPos[roomIdx];

    if (tl.getX() < 0 || tl.getY() < 0)
        return;

    // anchor is top-left
    Point origin(tl.getX(), tl.getY());

    char line1[LEGEND_BUFFER_SIZE];
    char line2[LEGEND_BUFFER_SIZE];

    snprintf(line1, sizeof(line1), "live: %d", lives);
    snprintf(line2, sizeof(line2), "Pts: %d", points);

    // Build line 3 with Unicode player icons using wide strings
    wchar_t wline3[LEGEND_BUFFER_SIZE];
    swprintf(wline3, LEGEND_BUFFER_SIZE, L"Inv: %lc=[%c] %lc=[%c]", 
             PLAYER_ONE_ICON, p1Inv == EMPTY_INVENTORY_SLOT ? EMPTY_INVENTORY_SLOT : p1Inv, 
             PLAYER_TWO_ICON, p2Inv == EMPTY_INVENTORY_SLOT ? EMPTY_INVENTORY_SLOT : p2Inv);

    auto pad16 = [](std::string& s) {
        if (s.size() < LEGEND_LINE_WIDTH)
            s.append(LEGEND_LINE_WIDTH - s.size(), ' ');
        else if (s.size() > LEGEND_LINE_WIDTH)
            s.resize(LEGEND_LINE_WIDTH);
    };

    std::string l1(line1), l2(line2); 
    pad16(l1); 
    pad16(l2);

    auto putLine = [&](int dy, const std::string& line) {
        for (int i = 0; i < LEGEND_LINE_WIDTH; ++i) {
            Point p(origin.getX() + i, origin.getY() + dy);
            if (p.getX() >= 0 && p.getX() < Screen::MAX_X && p.getY() >= 0 && p.getY() < Screen::MAX_Y) {
                p.draw(line[i]);
            }
        }
    };

    auto putWideLine = [&](int dy, const wchar_t* wline) {
        ScreenBuffer& buffer = ScreenBuffer::getInstance();
        int len = (int)wcslen(wline);
        if (len > LEGEND_LINE_WIDTH) len = LEGEND_LINE_WIDTH;
        for (int i = 0; i < len; ++i) {
            Point p(origin.getX() + i, origin.getY() + dy);
            if (p.getX() >= 0 && p.getX() < Screen::MAX_X && p.getY() >= 0 && p.getY() < Screen::MAX_Y) {
                buffer.setChar(p.getX(), p.getY(), wline[i]);
            }
        }
        // Pad remaining with spaces
        for (int i = len; i < 16; ++i) {
            Point p(origin.getX() + i, origin.getY() + dy);
            if (p.getX() >= 0 && p.getX() < Screen::MAX_X && p.getY() >= 0 && p.getY() < Screen::MAX_Y) {
                buffer.setChar(p.getX(), p.getY(), L' ');
            }
        }
    };

    putLine(0, l1);
    putLine(1, l2);
    putWideLine(2, wline3);
}

// Static method to scan all legends in the world
void Legend::scanAllLegends(std::vector<Screen>& world, Legend& legend) {
    legend.ensureRooms(world.size());
    for (int room = 0; room < (int)world.size(); ++room) {
        legend.locateLegendForRoom(room, world[room]);
    }
}