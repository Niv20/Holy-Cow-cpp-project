#pragma once
#include <vector>
#include <string>
#include "Point.h"

class Screen {
public:
    enum { MAX_X = 80, MAX_Y = 25 };
private:
    std::vector<std::string> m_map;

public:
    Screen(const std::vector<std::string>& mapData) : m_map(mapData) {}

    void draw() const;

    // Get what is at a specific coordinate (safe)
    char getCharAt(const Point& p) const;

    // Change what is at a specific coordinate (auto-pad)
    void setCharAt(const Point& p, char newChar);

    void erase(const Point& p);
};