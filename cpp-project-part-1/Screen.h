#pragma once
#include <vector>
#include <string>
#include <windows.h>
#include "Point.h"

class Screen {
public:
    enum { MAX_X = 80, MAX_Y = 25 };
private:
    struct SpecialChar { wchar_t ch; }; // single cell wrapper (can be extended later)
    std::vector<std::vector<SpecialChar>> m_grid; // 2D grid

    void initFromWideLines(const std::vector<std::wstring>& lines);

public:
    Screen(const std::vector<std::wstring>& mapData) { initFromWideLines(mapData); }
    Screen(const std::vector<std::string>& mapData); // UTF-8 input

    void draw() const;
    wchar_t getCharAt(const Point& p) const;
    void setCharAt(const Point& p, wchar_t newChar);
    void erase(const Point& p);

    // Redraw a single cell from grid to console (no grid change)
    void refreshCell(const Point& p) const;

    // Batch refresh for a set of cells
    void refreshCells(const std::vector<Point>& pts) const;
};