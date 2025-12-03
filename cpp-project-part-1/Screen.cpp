#include "Screen.h"
#include <iostream>
#include <cctype>
#include "Point.h"
#include "Glyph.h"
#include <iomanip>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <set>

namespace fs = std::filesystem;

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
    m_grid[p.y][p.x].ch = newChar;
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

void Screen::refreshCells(const std::vector<Point>& pts) const {
    for (auto& p : pts) refreshCell(p);
}

// Static helper for loading a level file
static std::vector<std::wstring> loadLevelFromFile(const std::string& filepath) {
    std::vector<std::wstring> board;
    std::ifstream inputFile(filepath, std::ios::binary);
    if (!inputFile.is_open()) return board;
    std::string content((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
    inputFile.close();
    if (content.size() >= 3 && (unsigned char)content[0]==0xEF && (unsigned char)content[1]==0xBB && (unsigned char)content[2]==0xBF) 
        content.erase(0,3);
    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();
        int wlen = MultiByteToWideChar(CP_UTF8,0,line.c_str(),(int)line.size(),nullptr,0);
        std::wstring wline(wlen,0);
        MultiByteToWideChar(CP_UTF8,0,line.c_str(),(int)line.size(),&wline[0],wlen);
        board.push_back(wline);
    }
    return board;
}

// Static method: Load all screens from files
std::vector<Screen> Screen::loadScreensFromFiles() {
    std::vector<std::string> mapFiles;
    try {
        for (const auto& entry : fs::directory_iterator(".")) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.rfind("adv-world",0)==0) {
                    auto extPos = filename.find_last_of('.');
                    if (extPos!=std::string::npos && filename.substr(extPos+1)=="screen") 
                        mapFiles.push_back(filename);
                }
            }
        }
    } catch(...) {}
    std::sort(mapFiles.begin(), mapFiles.end());
    std::vector<Screen> screens;
    for (auto& fn : mapFiles) {
        auto level = loadLevelFromFile(fn);
        if (!level.empty()) screens.emplace_back(level);
    }
    return screens;
}

// Instance method: Scan this screen's springs and switches
void Screen::scanScreenData(int roomIdx) {
    data_.springs.clear();
    data_.switches.clear();
    
    // Scan springs
    std::set<std::pair<int,int>> visited;
    for (int y = 0; y < MAX_Y; ++y) {
        for (int x = 0; x < MAX_X; ++x) {
            if (visited.count({x,y})) continue;
            Point p{x,y};
            if (!Glyph::isSpring(getCharAt(p))) continue;
            
            const int dirs[4][2] = {{0,-1},{0,1},{-1,0},{1,0}};
            for (int d=0; d<4; ++d) {
                int dx = dirs[d][0], dy = dirs[d][1];
                Point check{x + dx, y + dy};
                wchar_t ch = getCharAt(check);
                if (Glyph::isWall(ch)) {
                    std::vector<Point> springCells;
                    Point cur = p;
                    int odx = -dx, ody = -dy;
                    while (Glyph::isSpring(getCharAt(cur))) {
                        springCells.push_back(cur);
                        visited.insert({cur.x, cur.y});
                        cur.x += odx;
                        cur.y += ody;
                    }
                    if (!springCells.empty()) {
                        data_.springs.emplace_back(roomIdx, springCells, odx, ody, check);
                    }
                    break;
                }
            }
        }
    }
    
    // Scan switches
    for (int y = 0; y < MAX_Y; ++y) {
        for (int x = 0; x < MAX_X; ++x) {
            Point p{x, y};
            wchar_t ch = getCharAt(p);
            if (Glyph::isSwitch(ch)) {
                bool isOn = (ch == Glyph::Switch_On);
                data_.switches.emplace_back(roomIdx, p, isOn);
            }
        }
    }
}

// Static method: Scan all screens in the world
void Screen::scanAllScreens(std::vector<Screen>& world) {
    for (size_t room = 0; room < world.size(); ++room) {
        world[room].scanScreenData((int)room);
    }
}