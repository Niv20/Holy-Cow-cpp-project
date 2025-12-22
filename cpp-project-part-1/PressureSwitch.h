#pragma once
#include <vector>
#include "Point.h"

class Screen;

struct PressureButtonTarget {
    Point pos;
    wchar_t originalChar = L' ';

    PressureButtonTarget() = default;
    PressureButtonTarget(Point p, wchar_t orig) : pos(p), originalChar(orig) {}
};

class PressureButton {
public:
    int roomIdx;
    Point pos;
    std::vector<PressureButtonTarget> targets;

    PressureButton(int room, Point p);

    void setTargets(const std::vector<Point>& points, const Screen& screen);

    static PressureButton* findAt(Screen& screen, const Point& p);
};
