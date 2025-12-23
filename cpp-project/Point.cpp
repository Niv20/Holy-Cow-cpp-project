#include "Point.h"

void Point::setDirection(MoveDirection dir) {
    switch (dir) {
    case MoveDirection::Up:    diff_x_ = 0;  diff_y_ = -1; break;
    case MoveDirection::Right: diff_x_ = 1;  diff_y_ = 0;  break;
    case MoveDirection::Down:  diff_x_ = 0;  diff_y_ = 1;  break;
    case MoveDirection::Left:  diff_x_ = -1; diff_y_ = 0;  break;
    case MoveDirection::Stay:  diff_x_ = 0;  diff_y_ = 0;  break;
    }
}