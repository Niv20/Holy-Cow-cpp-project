#pragma once
#include <vector>
#include <string>
#include <map>
#include <windows.h>
#include "Point.h"
#include "Spring.h"
#include "Switch.h"
#include "PressureSwitch.h"
#include "SpecialDoor.h"
#include "Obstacle.h"
#include "ScreenMetadata.h"

// Forward declarations
class Legend;
class RoomConnections;
class Riddle;
class RiddleKey;

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
        std::vector<PressureButton> pressureButtons;
        std::vector<SpecialDoor> doors;
        std::vector<Obstacle> obstacles;
        std::vector<DarkZone> darkZones;  // Dark areas in this screen
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
    
    // Get metadata for this screen
    const ScreenMetadata& getMetadata() const { return metadata_; }
    ScreenMetadata& getMetadataMutable() { return metadata_; }
    
    // Check if a point is in a dark zone (and not yet lit)
    bool isInDarkZone(const Point& p) const;
    
    // Light up a dark zone containing the given point
    void lightDarkZone(const Point& p);
    
    // Render the message box content from metadata (called once when entering room)
    void renderMessageBox(const std::string& line1, const std::string& line2, const std::string& line3);
    
    // Structure to hold loaded screen with metadata
    struct LoadedScreen {
        std::vector<std::wstring> screenLines;  // The visual screen content (first 25 lines)
        ScreenMetadata metadata;                 // Parsed metadata from file
    };
    
    // Load a single screen file and separate screen content from metadata
    static LoadedScreen loadScreenFile(const std::string& filepath);
    
    // Parse metadata section from lines
    static ScreenMetadata parseMetadata(const std::vector<std::string>& metadataLines);

private:
    Data data_;
    ScreenMetadata metadata_;
};