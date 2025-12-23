#pragma once
#include <windows.h>

// This file is based on Amir's tirgol

// Movement direction for Point
enum class MoveDirection {
    Up = 0,
    Right = 1,
    Down = 2,
    Left = 3,
    Stay = 4
};

// Helper function to convert int to MoveDirection
inline MoveDirection intToMoveDirection(int dir) {
    switch (dir) {
        case 0: return MoveDirection::Up;
        case 1: return MoveDirection::Right;
        case 2: return MoveDirection::Down;
        case 3: return MoveDirection::Left;
        default: return MoveDirection::Stay;
    }
}

class Point {
private:
    int x_ = 1;
    int y_ = 1;
    int diff_x_ = 0; // Movement direction X
    int diff_y_ = 0; // Movement direction Y

public:
    Point(int _x, int _y) : x_(_x), y_(_y) {}
    Point() {}

    // Getters
    int getX() const { return x_; }
    int getY() const { return y_; }
    int getDiffX() const { return diff_x_; }
    int getDiffY() const { return diff_y_; }
    
    // Setters
    void setX(int val) { x_ = val; }
    void setY(int val) { y_ = val; }
    void setDiffX(int val) { diff_x_ = val; }
    void setDiffY(int val) { diff_y_ = val; }

    void move() { x_ += diff_x_; y_ += diff_y_; }

    // Sets the direction based on keyboard input
    void setDirection(MoveDirection dir);

    void draw(char ch) const {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        COORD pos{ (SHORT)x_, (SHORT)y_ };
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