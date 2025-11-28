#pragma once
#include <windows.h>
#include "utils.h"

// Represents a 2D point on the console
struct Point {
    int x = 1;
    int y = 1;
    int diff_x = 0; // Movement direction X
    int diff_y = 0; // Movement direction Y

    Point(int _x, int _y) : x(_x), y(_y) {}
    Point() {}

    void move() { x += diff_x; y += diff_y; }

    // Sets the direction based on keyboard input (0=UP, 1=RIGHT, etc.)
    void setDirection(int dir);

    void draw(char ch) const {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        COORD pos{ (SHORT)x, (SHORT)y };
        SetConsoleCursorPosition(hOut, pos);
        wchar_t wc;
        unsigned char uc = static_cast<unsigned char>(ch);
        // Preserve legacy mapping for specific CP437 codes
        if (uc == 148) wc = L'ö';
        else if (uc == 129) wc = L'ü';
        else wc = (wchar_t)uc;
        DWORD written; WriteConsoleW(hOut, &wc, 1, &written, nullptr);
    }
};