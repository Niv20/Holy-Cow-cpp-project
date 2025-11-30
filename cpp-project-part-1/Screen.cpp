#include "Screen.h"
#include <iostream>
#include <cctype>
#include "Point.h"
#include "Glyph.h" // was Tiles.h
#include <iomanip>

void Screen::initFromWideLines(const std::vector<std::wstring>& lines) {
    m_grid.clear();
    m_grid.resize(MAX_Y, std::vector<SpecialChar>(MAX_X, SpecialChar{ Glyph::Empty }));
    int yLimit = std::min<int>(MAX_Y, (int)lines.size());
    for (int y = 0; y < yLimit; ++y) {
        const std::wstring& src = lines[y];
        int xLimit = std::min<int>(MAX_X, (int)src.size());
        for (int x = 0; x < xLimit; ++x) {
            m_grid[y][x].ch = src[x];
        }
    }
}

Screen::Screen(const std::vector<std::string>& mapData) {
    std::vector<std::wstring> widened;
    widened.reserve(mapData.size());
    for (auto& line : mapData) {
        int wlen = MultiByteToWideChar(CP_UTF8,0,line.c_str(),(int)line.size(),nullptr,0);
        std::wstring wline(wlen,0);
        MultiByteToWideChar(CP_UTF8,0,line.c_str(),(int)line.size(),&wline[0],wlen);
        widened.push_back(wline);
    }
    initFromWideLines(widened);
}

void Screen::draw() const {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    for (int y = 0; y < MAX_Y; ++y) {
        std::wstring out;
        out.reserve(MAX_X);
        for (int x = 0; x < MAX_X; ++x) {
            wchar_t c = m_grid[y][x].ch;
            out.push_back(c);
        }
        COORD linePos{0,(SHORT)y};
        SetConsoleCursorPosition(hOut, linePos);
        DWORD written; WriteConsoleW(hOut, out.c_str(), (DWORD)out.size(), &written, nullptr);
    }
}

wchar_t Screen::getCharAt(const Point& p) const {
    if (p.x < 0 || p.x >= MAX_X || p.y < 0 || p.y >= MAX_Y) return Glyph::Empty;
    return m_grid[p.y][p.x].ch;
}

void Screen::setCharAt(const Point& p, wchar_t newChar) {
    if (p.x < 0 || p.x >= MAX_X || p.y < 0 || p.y >= MAX_Y) return;
    m_grid[p.y][p.x].ch = newChar; // no immediate console write
}

void Screen::erase(const Point& p) { setCharAt(p, Glyph::Empty); }

void Screen::refreshCell(const Point& p) const {
    if (p.x < 0 || p.x >= MAX_X || p.y < 0 || p.y >= MAX_Y) return;
    wchar_t c = m_grid[p.y][p.x].ch;
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos{ (SHORT)p.x, (SHORT)p.y };
    SetConsoleCursorPosition(hOut, pos);
    DWORD written; WriteConsoleW(hOut, &c, 1, &written, nullptr);
}