#pragma once
#include <iostream>
#include "utils.h"

// Represents a 2D point on the console
struct Point {
    int x = 1;
    int y = 1;
    int diff_x = 0; // Movement direction X
    int diff_y = 0; // Movement direction Y

    Point(int _x, int _y) : x(_x), y(_y) {}
    Point() {}

    void move() {
        x += diff_x;
        y += diff_y;
    }

    // Sets the direction based on keyboard input (0=UP, 1=RIGHT, etc.)
    void setDirection(int dir);

    void draw(char ch) const {
        gotoxy(x, y);
        std::cout << ch;
    }
};