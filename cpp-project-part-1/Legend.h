#pragma once
#include <string>
#include <vector>
#include "Screen.h"
#include "Point.h"
#include "Tiles.h"

class Legend {
    // Legend anchor per room (top-left) discovered by 'L'
    // We store per-room position to keep it persistent across redraws
    std::vector<Point> roomLegendPos; // indexed by room idx

public:
    Legend() = default;

    void ensureRooms(size_t count) {
        if (roomLegendPos.size() < count) roomLegendPos.resize(count, Point{ -1, -1 });
    }

    // Scan the screen for 'L' and remember its position for the given room
    void locateLegendForRoom(int roomIdx, const Screen& s) {
        if (roomIdx < 0) return;
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

    // Draw a 3x20 legend box at the remembered top-left position
    void drawLegend(int roomIdx, Screen& s, int lives, int points, char p1Inv, char p2Inv) {
        ensureRooms(static_cast<size_t>(roomIdx + 1));
        Point tl = roomLegendPos[roomIdx];
        if (tl.x < 0 || tl.y < 0) return; // no legend anchor, nothing to draw

        auto putLine = [&](int dy, const std::string& line) {
            for (int i = 0; i < 20 && i < (int)line.size(); ++i) {
                Point p{ tl.x + i, tl.y + dy };
                if (p.x >= 0 && p.x < Screen::MAX_X && p.y >= 0 && p.y < Screen::MAX_Y) {
                    s.setCharAt(p, line[i]);
                }
            }
        };

        // Format lines (pad to 20)
        char buf1[32];
        char buf2[32];
        char buf3[32];
        snprintf(buf1, sizeof(buf1), "live: %-10d", lives);
        snprintf(buf2, sizeof(buf2), "Pts: %-11d", points);

        // Inventory line: ö=[!] ü=[ ] (using the configured player symbols)
        char inv1 = (p1Inv == ' ') ? ' ' : p1Inv;
        char inv2 = (p2Inv == ' ') ? ' ' : p2Inv;
        snprintf(buf3, sizeof(buf3), "Inv: %c=[%c] %c=[%c]",
                 Tiles::First_Player, inv1, Tiles::Second_Player, inv2);

        std::string l1(buf1), l2(buf2), l3(buf3);
        l1.resize(20, ' ');
        l2.resize(20, ' ');
        l3.resize(20, ' ');

        putLine(0, l1);
        putLine(1, l2);
        putLine(2, l3);
    }
};
