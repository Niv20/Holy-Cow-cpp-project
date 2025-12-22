#include "PressureSwitch.h"
#include "Screen.h"
#include "Glyph.h"

PressureButton::PressureButton(int room, Point p) : roomIdx(room), pos(p) {}

void PressureButton::setTargets(const std::vector<Point>& points, const Screen& screen) {
    targets.clear();
    targets.reserve(points.size());
    for (const auto& pt : points) {
        wchar_t original = screen.getCharAt(pt);
        targets.emplace_back(pt, original == 0 ? Glyph::Empty : original);
    }
}

PressureButton* PressureButton::findAt(Screen& screen, const Point& p) {
    auto& data = screen.getDataMutable();
    for (auto& sw : data.pressureButtons) {
        if (sw.pos.x == p.x && sw.pos.y == p.y) {
            return &sw;
        }
    }
    return nullptr;
}
