#include "Point.h"

void Point::setDirection(int dir) {
    switch (dir) {
    case 0: diff_x = 0; diff_y = -1; break; // UP
    case 1: diff_x = 1; diff_y = 0; break;  // RIGHT
    case 2: diff_x = 0; diff_y = 1; break;  // DOWN
    case 3: diff_x = -1; diff_y = 0; break; // LEFT
    case 4: diff_x = 0; diff_y = 0; break;  // STAY
    }
}