#include "Screen.h"
#include <iostream>
#include <cctype>
#include "Point.h"
#include "Tiles.h"
#include <iomanip>

Screen::Screen(const std::vector<std::string>& mapData) {
    m_map.reserve(mapData.size());
    for (auto& line : mapData) {
        int wlen = MultiByteToWideChar(CP_UTF8,0,line.c_str(),(int)line.size(),nullptr,0);
        std::wstring wline(wlen,0);
        MultiByteToWideChar(CP_UTF8,0,line.c_str(),(int)line.size(),&wline[0],wlen);
        m_map.push_back(wline);
    }
}

void Screen::draw() const {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    for (size_t y = 0; y < m_map.size(); ++y) {
        std::wstring out;
        out.reserve(m_map[y].size());
        for (wchar_t c : m_map[y]) {
            if (Tiles::isRoomTransition(c)) out.push_back(L' '); else out.push_back(c);
        }
        if (out.size() < MAX_X) out.append(MAX_X - out.size(), L' '); else if(out.size()>MAX_X) out.resize(MAX_X);
        COORD linePos{0,(SHORT)y};
        SetConsoleCursorPosition(hOut, linePos);
        DWORD written; WriteConsoleW(hOut, out.c_str(), (DWORD)out.size(), &written, nullptr);
    }
}

wchar_t Screen::getCharAt(const Point& p) const {
    if (p.x < 0 || p.x >= MAX_X || p.y < 0 || p.y >= MAX_Y) return L' ';
    if (p.y >= (int)m_map.size() || p.x >= (int)m_map[p.y].size()) return L' ';
    return m_map[p.y][p.x];
}

void Screen::setCharAt(const Point& p, wchar_t newChar) {
    if (p.x < 0 || p.x >= MAX_X || p.y < 0 || p.y >= MAX_Y) return;
    if (p.y >= (int)m_map.size()) return;
    if (p.x >= (int)m_map[p.y].size()) m_map[p.y].resize(MAX_X, L' ');
    m_map[p.y][p.x] = newChar;
    // Instrumentation: log every set
    std::cerr << "[setCharAt] room=? x=" << p.x << " y=" << p.y << " char=0x" << std::hex << std::setw(4) << std::setfill('0') << (int)newChar << std::dec << " ('" << (newChar < 128 ? (char)newChar : '?') << "')" << std::endl;
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos{(SHORT)p.x,(SHORT)p.y}; SetConsoleCursorPosition(hOut,pos);
    DWORD written; WriteConsoleW(hOut,&newChar,1,&written,nullptr);
}

void Screen::erase(const Point& p) { setCharAt(p, L' '); }