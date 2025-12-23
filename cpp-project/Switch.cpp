#include "Switch.h"
#include "Screen.h"

// Static: Find switch at position
SwitchData* SwitchData::findAt(Screen& screen, const Point& p) {
    auto& data = screen.getDataMutable();
    for (auto& sw : data.switches) {
        Point swPos = sw.getPos();
        if (swPos.getX() == p.getX() && swPos.getY() == p.getY()) {
            return &sw;
        }
    }
    return nullptr;
}
