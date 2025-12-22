#include "Switch.h"
#include "Screen.h"

// Static: Find switch at position
SwitchData* SwitchData::findAt(Screen& screen, const Point& p) {
    auto& data = screen.getDataMutable();
    for (auto& sw : data.switches) {
        if (sw.pos.x == p.x && sw.pos.y == p.y) {
            return &sw;
        }
    }
    return nullptr;
}
