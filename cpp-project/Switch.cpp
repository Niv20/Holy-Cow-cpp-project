#include "Switch.h"
#include "Screen.h"

// Static: Find switch at position
SwitchData* SwitchData::findAt(Screen& screen, const Point& p) {
    auto& data = screen.getDataMutable();
    for (auto& sw : data.switches) {
        Point swPos = sw.getPos();
        if (swPos.x == p.x && swPos.y == p.y) {
            return &sw;
        }
    }
    return nullptr;
}
