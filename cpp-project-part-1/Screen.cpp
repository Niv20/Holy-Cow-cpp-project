#include "Screen.h"
#include <iostream>
#include <cctype>
#include "Point.h"

void Screen::draw() const {

    gotoxy(0, 0);

    for (const auto& line : m_map) {
        for (char c : line) {
            // Hides the numbers (0-9) from the player
            if (isdigit(c)) {
                std::cout << ' ';
            }
            else {
                std::cout << c;
            }
        }
    }
}

char Screen::getCharAt(const Point& p) const {
    
	// Out of bounds check
    if (p.x < 0 || p.x >= MAX_X || p.y < 0 || p.y >= MAX_Y) 
        return ' ';

    return m_map[p.y][p.x];
}

void Screen::setCharAt(const Point& p, char newChar) {
    m_map[p.y][p.x] = newChar;
    p.draw(newChar);
}

void Screen::erase(const Point& p) {
    setCharAt(p, ' ');
}