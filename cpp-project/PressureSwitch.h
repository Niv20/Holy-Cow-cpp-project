#pragma once
#include <vector>
#include "Point.h"

class Screen;

class PressureButtonTarget {
private:
    Point pos_;
    wchar_t originalChar_ = L' ';

public:
    PressureButtonTarget() = default;
    PressureButtonTarget(Point p, wchar_t orig) : pos_(p), originalChar_(orig) {}
    
    Point getPos() const { return pos_; }
    wchar_t getOriginalChar() const { return originalChar_; }
};

class PressureButton {
private:
    int roomIdx_;
    Point pos_;
    std::vector<PressureButtonTarget> targets_;

public:
    PressureButton(int room, Point p);

    // Getters
    int getRoomIdx() const { return roomIdx_; }
    Point getPos() const { return pos_; }
    const std::vector<PressureButtonTarget>& getTargets() const { return targets_; }

    void setTargets(const std::vector<Point>& points, const Screen& screen);

    static PressureButton* findAt(Screen& screen, const Point& p);
};
