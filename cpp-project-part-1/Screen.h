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
    // TODO: להבין לעומק מה כתוב כאן
    Screen(const std::vector<std::string>& mapData) : m_map(mapData) {}

    void draw() const;

    // Get what is at a specific coordinate
    char getCharAt(const Point& p) const;

    // Change what is at a specific coordinate
    void setCharAt(const Point& p, char newChar);

    void erase(const Point& p);
};