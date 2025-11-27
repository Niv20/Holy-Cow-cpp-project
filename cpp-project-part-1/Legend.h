#pragma once
#include <string>
#include <vector>
#include "Screen.h"
#include "Point.h"
#include "Tiles.h"

class Legend {
    std::vector<Point> roomLegendPos; // indexed by room idx
public:
    Legend() = default;

    void ensureRooms(size_t count) {
        if (roomLegendPos.size() < count) roomLegendPos.resize(count, Point{ -1, -1 });
    }

    void locateLegendForRoom(int roomIdx, const Screen& s) {
        if (roomIdx < 0) return;
        ensureRooms(static_cast<size_t>(roomIdx + 1));
        for (int y = 0; y < Screen::MAX_Y; ++y) {
            for (int x = 0; x < Screen::MAX_X; ++x) {
                Point p{ x, y };
                if (s.getCharAt(p) == 'L') { // anchor
                    roomLegendPos[roomIdx] = p;
                    return;
                }
            }
        }
        roomLegendPos[roomIdx] = Point{ -1, -1 }; // not found
    }

    void drawLegend(int roomIdx, Screen& s, int lives, int points, char p1Inv, char p2Inv) {
        ensureRooms(static_cast<size_t>(roomIdx + 1));
        Point tl = roomLegendPos[roomIdx];
        if (tl.x < 0 || tl.y < 0) return;

        // Shift one more column left (x -2) and keep previous y offset (-1)
        Point origin{ tl.x - 2, tl.y - 1 };

        char line1[32];
        char line2[32];
        char line3[32];
        snprintf(line1, sizeof(line1), "live: %d", lives);
        // Points line with space and no zero padding (e.g. "Pts: 0")
        snprintf(line2, sizeof(line2), "Pts: %d", points);
        char inv1 = (p1Inv == ' ') ? ' ' : p1Inv;
        char inv2 = (p2Inv == ' ') ? ' ' : p2Inv;
        snprintf(line3, sizeof(line3), "Inv: a=[%c] b=[%c]", inv1, inv2);

        auto pad16 = [](std::string& s) { if (s.size() < 16) s.append(16 - s.size(), ' '); else if (s.size() > 16) s.resize(16); };
        std::string l1(line1), l2(line2), l3(line3);
        pad16(l1); pad16(l2); pad16(l3);

        auto putLine = [&](int dy, const std::string& line) {
            for (int i = 0; i < 16; ++i) {
                Point p{ origin.x + i, origin.y + dy };
                if (p.x >= 0 && p.x < Screen::MAX_X && p.y >= 0 && p.y < Screen::MAX_Y) {
                    s.setCharAt(p, line[i]);
                }
            }
        };

        putLine(0, l1);
        putLine(1, l2);
        putLine(2, l3);
    }
};
