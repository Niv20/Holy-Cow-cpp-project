#pragma once
#include <vector>
#include <string>
#include <windows.h>
#include "Point.h"

class Screen {
public:
    enum { MAX_X = 80, MAX_Y = 25 };
private:
    std::vector<std::wstring> m_map; // wide map

public:
    Screen(const std::vector<std::wstring>& mapData) : m_map(mapData) {}
    // Overload: accept narrow strings (UTF-8 assumed) and widen
    Screen(const std::vector<std::string>& mapData);

    void draw() const;

    // Get what is at a specific coordinate (safe)
    wchar_t getCharAt(const Point& p) const;

    // Change what is at a specific coordinate (auto-pad)
    void setCharAt(const Point& p, wchar_t newChar);

    void erase(const Point& p);
};