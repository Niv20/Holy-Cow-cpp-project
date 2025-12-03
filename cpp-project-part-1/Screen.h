#pragma once
#include <vector>
#include <string>
#include <map>
#include <windows.h>
#include "Point.h"
#include "Spring.h"
#include "Switch.h"
#include "SpecialDoor.h"
#include "Obstacle.h"

// Forward declarations
class Legend;
class RoomConnections;
class Riddle;
struct RiddleKey;

class Screen {
public:
    enum { MAX_X = 80, MAX_Y = 25 };
private:
    struct SpecialChar { wchar_t ch; }; 
    std::vector<std::vector<SpecialChar>> m_grid;

    void initFromWideLines(const std::vector<std::wstring>& lines);

public:
    // Per-screen scanned data holder
    struct Data {
        std::vector<SpringData> springs;
        std::vector<SwitchData> switches;
        std::vector<SpecialDoor> doors;
        std::vector<Obstacle> obstacles;
    };

    Screen(const std::vector<std::wstring>& mapData) { initFromWideLines(mapData); }
    Screen(const std::vector<std::string>& mapData);

    void draw() const;
    wchar_t getCharAt(const Point& p) const;
    void setCharAt(const Point& p, wchar_t newChar);
    void erase(const Point& p);
    void refreshCell(const Point& p) const;
    void refreshCells(const std::vector<Point>& pts) const;

    // Access per-screen data
    const Data& getData() const { return data_; }
    Data& getDataMutable() { return data_; }

    // Static methods for loading and scanning screens
    static std::vector<Screen> loadScreensFromFiles();
    
    // Scan ALL data for all screens: springs, switches, doors, obstacles, riddles, legends
    static void scanAllScreens(std::vector<Screen>& world, 
                                const RoomConnections& roomConnections,
                                std::map<RiddleKey, Riddle*>& riddlesByPosition,
                                Legend& legend);
    
    // Scan this screen's data (springs, switches)
    void scanScreenData(int roomIdx);

private:
    Data data_;
};