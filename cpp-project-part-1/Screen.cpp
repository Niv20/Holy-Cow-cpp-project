#include "Screen.h"
#include <iostream>
#include <cctype>
#include "Point.h"
#include "Tiles.h"

void Screen::draw() const {
    gotoxy(0, 0);

    for (const auto& line : m_map) {
        // Build a line replacing room transition digits with empty tiles
        std::string out;
        out.reserve(line.size());
        for (unsigned char c : line) {
            if (Tiles::isRoomTransition((char)c)) out.push_back(Tiles::Empty); else out.push_back((char)c);
        }
        std::cout << out << '\n'; // ensure newline between rows
    }
}

char Screen::getCharAt(const Point& p) const {
    if (p.x < 0 || p.x >= MAX_X || p.y < 0 || p.y >= MAX_Y) 
        return ' ';
    if (p.y >= (int)m_map.size() || p.x >= (int)m_map[p.y].size()) return ' ';
    return m_map[p.y][p.x];
}

void Screen::setCharAt(const Point& p, char newChar) {
    if (p.x < 0 || p.x >= MAX_X || p.y < 0 || p.y >= MAX_Y) return;
    if (p.y >= (int)m_map.size()) return;
    if (p.x >= (int)m_map[p.y].size()) m_map[p.y].resize(MAX_X, ' ');
    m_map[p.y][p.x] = newChar;
    p.draw(newChar);
}

void Screen::erase(const Point& p) {
    setCharAt(p, ' ');
}