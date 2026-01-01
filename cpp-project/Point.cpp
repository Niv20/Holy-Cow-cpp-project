#include "Point.h"
#include "ScreenBuffer.h"

void Point::setDirection(MoveDirection dir) {
    switch (dir) {
    case MoveDirection::Up:    diff_x_ = 0;  diff_y_ = -1; break;
    case MoveDirection::Right: diff_x_ = 1;  diff_y_ = 0;  break;
    case MoveDirection::Down:  diff_x_ = 0;  diff_y_ = 1;  break;
    case MoveDirection::Left:  diff_x_ = -1; diff_y_ = 0;  break;
    case MoveDirection::Stay:  diff_x_ = 0;  diff_y_ = 0;  break;
    }
}

void Point::draw(char ch) const {
    wchar_t wc;
    unsigned char uc = static_cast<unsigned char>(ch);
    // Preserve legacy mapping for specific CP437 codes
    if (uc == 148) wc = L'\u00F6'; // ö
    else if (uc == 129) wc = L'\u00FC'; // ü
    else wc = (wchar_t)uc;
    ScreenBuffer::getInstance().setChar(x_, y_, wc);
}