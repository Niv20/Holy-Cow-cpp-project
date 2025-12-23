#include "PressureSwitch.h"
#include "Screen.h"
#include "Glyph.h"

PressureButton::PressureButton(int room, Point p) : roomIdx_(room), pos_(p) {}

void PressureButton::setTargets(const std::vector<Point>& points, const Screen& screen) {
    targets_.clear();
    targets_.reserve(points.size());
    for (const auto& pt : points) {
        wchar_t original = screen.getCharAt(pt);
        targets_.emplace_back(pt, original == 0 ? Glyph::Empty : original);
    }
}

PressureButton* PressureButton::findAt(Screen& screen, const Point& p) {
    auto& data = screen.getDataMutable();
    for (auto& sw : data.pressureButtons) {
        Point swPos = sw.getPos();
        if (swPos.x == p.x && swPos.y == p.y) {
            return &sw;
        }
    }
    return nullptr;
}
