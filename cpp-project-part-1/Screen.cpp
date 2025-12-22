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
#include "SpecialDoor.h"
#include "Obstacle.h"
#include "Riddle.h"
#include "Legend.h"
#include "RoomConnections.h"
#include "FileParser.h"

// This file written by AI

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

// Static method: Load a screen file and separate content from metadata
Screen::LoadedScreen Screen::loadScreenFile(const std::string& filepath) {
    LoadedScreen result;
    
    std::ifstream inputFile(filepath, std::ios::binary);
    if (!inputFile.is_open()) {
        FileParser::reportError("Cannot open screen file: " + filepath);
        return result;
    }
    
    std::string content((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
    inputFile.close();
    
    // Remove UTF-8 BOM if present
    if (content.size() >= 3 && (unsigned char)content[0]==0xEF && (unsigned char)content[1]==0xBB && (unsigned char)content[2]==0xBF) 
        content.erase(0,3);
    
    std::istringstream stream(content);
    std::string line;
    std::vector<std::string> allLines;
    
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        allLines.push_back(line);
    }
    
    // Find metadata separator
    int metadataStart = -1;
    for (size_t i = 0; i < allLines.size(); ++i) {
        std::string trimmed = FileParser::trim(allLines[i]);
        if (trimmed == "=== METADATA ===") {
            metadataStart = (int)i;
            break;
        }
    }
    
    // Separate screen lines (first 25 or until metadata) from metadata lines
    int screenEndLine;
    if (metadataStart >= 0) {
        screenEndLine = (metadataStart < MAX_Y) ? metadataStart : MAX_Y;
    } else {
        screenEndLine = ((int)allLines.size() < MAX_Y) ? (int)allLines.size() : MAX_Y;
    }
    
    for (int i = 0; i < screenEndLine; ++i) {
        const std::string& srcLine = allLines[i];
        int wlen = MultiByteToWideChar(CP_UTF8, 0, srcLine.c_str(), (int)srcLine.size(), nullptr, 0);
        std::wstring wline(wlen, 0);
        MultiByteToWideChar(CP_UTF8, 0, srcLine.c_str(), (int)srcLine.size(), &wline[0], wlen);
        result.screenLines.push_back(wline);
    }
    
    // Parse metadata if present
    if (metadataStart >= 0 && metadataStart + 1 < (int)allLines.size()) {
        std::vector<std::string> metadataLines(allLines.begin() + metadataStart + 1, allLines.end());
        result.metadata = parseMetadata(metadataLines);
    }

    // Find 'T' marker for message box, store its position, and remove it from the grid
    for (int y = 0; y < (int)result.screenLines.size(); ++y) {
        for (int x = 0; x < (int)result.screenLines[y].size(); ++x) {
            if (result.screenLines[y][x] == L'T') {
                result.metadata.messageBox.anchorPos = Point(x, y);
                // Auto-detect box width
                for (int w = 1; x + w < (int)result.screenLines[y].size(); ++w) {
                    if (result.screenLines[y][x + w] == L'│' || result.screenLines[y][x + w] == L'|') {
                        result.metadata.messageBox.boxWidth = w;
                        break;
                    }
                }
                result.screenLines[y][x] = L' '; // Remove 'T' from grid
                break; 
            }
        }
    }
    
    return result;
}

// Static method: Parse metadata section
ScreenMetadata Screen::parseMetadata(const std::vector<std::string>& metadataLines) {
    ScreenMetadata metadata;
    
    DoorMetadata* currentDoor = nullptr;
    PressureButtonMetadata* currentPressureButton = nullptr;
    bool inDarkZones = false;
    int lineNum = 0;
    
    for (const auto& rawLine : metadataLines) {
        lineNum++;
        std::string line = FileParser::trim(rawLine);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;
        
        // End marker
        if (FileParser::startsWith(line, "---")) {
            if (currentDoor) {
                metadata.doors.push_back(*currentDoor);
                delete currentDoor;
                currentDoor = nullptr;
            }
            if (currentPressureButton) {
                metadata.pressureButtons.push_back(*currentPressureButton);
                delete currentPressureButton;
                currentPressureButton = nullptr;
            }
            inDarkZones = false;
            continue;
        }
        
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        
        // Door definition
        if (cmd == "DOOR") {
            if (currentDoor) {
                metadata.doors.push_back(*currentDoor);
                delete currentDoor;
            }
            if (currentPressureButton) {
                metadata.pressureButtons.push_back(*currentPressureButton);
                delete currentPressureButton;
                currentPressureButton = nullptr;
            }
            currentDoor = new DoorMetadata();
            int x, y;
            iss >> x >> y;
            currentDoor->position = Point(x, y);
        }
        else if (cmd == "KEYS" && currentDoor) {
            char key;
            while (iss >> key) {
                if (std::islower(key)) {
                    currentDoor->requiredKeys.push_back(key);
                }
            }
        }
        else if (cmd == "SWITCH" && currentDoor) {
            int sx, sy, state;
            iss >> sx >> sy >> state;
            currentDoor->switchRequirements.push_back({Point(sx, sy), (bool)state});
        }
        else if (cmd == "TARGET" && currentDoor) {
            int room, tx, ty;
            iss >> room >> tx >> ty;
            currentDoor->targetRoom = room;
            currentDoor->targetPosition = Point(tx, ty);
        }
        // Pressure button definitions
        else if (cmd == "PBUTTON") {
            if (currentPressureButton) {
                metadata.pressureButtons.push_back(*currentPressureButton);
                delete currentPressureButton;
            }
            if (currentDoor) {
                metadata.doors.push_back(*currentDoor);
                delete currentDoor;
                currentDoor = nullptr;
            }
            currentPressureButton = new PressureButtonMetadata();
            int px, py;
            iss >> px >> py;
            currentPressureButton->position = Point(px, py);
        }
        else if (cmd == "CLEAR" && currentPressureButton) {
            int cx, cy;
            iss >> cx >> cy;
            currentPressureButton->clearTargets.emplace_back(cx, cy);
        }
        // Dark zone definitions
        else if (cmd == "DARK") {
            int x1, y1, x2, y2;
            iss >> x1 >> y1 >> x2 >> y2;
            metadata.darkZones.emplace_back(x1, y1, x2, y2);
        }
        else if (cmd == "DARKZONES") {
            inDarkZones = true;
        }
        else if (cmd == "ZONE" && inDarkZones) {
            int x1, y1, x2, y2;
            iss >> x1 >> y1 >> x2 >> y2;
            metadata.darkZones.emplace_back(x1, y1, x2, y2);
        }
        // Connection definitions - store in connections map
        else if (cmd == "CONNECT") {
            std::string dir;
            int targetRoom;
            iss >> dir >> targetRoom;
            // Store in both formats for compatibility
            metadata.connectionOverrides.push_back({dir, targetRoom});
            // Convert to uppercase for consistent lookup
            std::transform(dir.begin(), dir.end(), dir.begin(), ::toupper);
            metadata.connections[dir] = targetRoom;
        }
        // Message box definitions
        else if (cmd == "LINE1") {
            // Get the rest of the line after "LINE1 "
            std::string text;
            std::getline(iss, text);
            // Trim leading whitespace
            size_t start = text.find_first_not_of(" \t");
            if (start != std::string::npos) {
                text = text.substr(start);
            } else {
                text = "";
            }
            metadata.messageBox.line1 = text;
            metadata.messageBox.hasMessage = true;
        }
        else if (cmd == "LINE2") {
            std::string text;
            std::getline(iss, text);
            size_t start = text.find_first_not_of(" \t");
            if (start != std::string::npos) {
                text = text.substr(start);
            } else {
                text = "";
            }
            metadata.messageBox.line2 = text;
            metadata.messageBox.hasMessage = true;
        }
        else if (cmd == "LINE3") {
            std::string text;
            std::getline(iss, text);
            size_t start = text.find_first_not_of(" \t");
            if (start != std::string::npos) {
                text = text.substr(start);
            } else {
                text = "";
            }
            metadata.messageBox.line3 = text;
            metadata.messageBox.hasMessage = true;
        }
    }
    
    // Handle unclosed door
    if (currentDoor) {
        metadata.doors.push_back(*currentDoor);
        delete currentDoor;
    }
    if (currentPressureButton) {
        metadata.pressureButtons.push_back(*currentPressureButton);
        delete currentPressureButton;
    }
    
    return metadata;
}

// Helper: get executable directory for robust resource loading
static fs::path getExeDir() {
    wchar_t buffer[MAX_PATH];
    DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (len == 0) return fs::current_path();
    fs::path exePath(buffer);
    return exePath.parent_path();
}

// Static method: Load all screens from files
std::vector<Screen> Screen::loadScreensFromFiles() {
    std::vector<std::string> mapFiles;
    fs::path baseDir = getExeDir();
    // Also consider parent of exe dir and current path
    std::vector<fs::path> dirs{ baseDir, baseDir.parent_path(), fs::current_path() };
    try {
        for (const auto& dir : dirs) {
            for (const auto& entry : fs::directory_iterator(dir)) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    if (filename.rfind("adv-world",0)==0) {
                        auto extPos = filename.find_last_of('.');
                        if (extPos!=std::string::npos && filename.substr(extPos+1)=="screen") 
                            mapFiles.push_back(entry.path().string());
                    }
                }
            }
        }
    } catch(const std::exception& e) {
        FileParser::reportError(std::string("Error scanning directories: ") + e.what());
    } catch(...) {
        FileParser::reportError("Unknown error scanning directories");
    }
    
    // Remove duplicates (same file found in multiple directories)
    std::sort(mapFiles.begin(), mapFiles.end());
    mapFiles.erase(std::unique(mapFiles.begin(), mapFiles.end()), mapFiles.end());
    
    std::vector<Screen> screens;
    for (auto& fullPath : mapFiles) {
        LoadedScreen loaded = loadScreenFile(fullPath);
        if (!loaded.screenLines.empty()) {
            screens.emplace_back(loaded.screenLines);
            // Store metadata in the screen
            screens.back().metadata_ = loaded.metadata;
            // Copy dark zones to data_ for runtime access
            screens.back().data_.darkZones = loaded.metadata.darkZones;
        } else {
            FileParser::reportError("Failed to load screen file: " + fullPath);
        }
    }
    
    if (screens.empty()) {
        FileParser::reportError("No level screens found. Place adv-world*.screen next to the EXE or project root.");
    }
    
    return screens;
}

// Instance method: Scan this screen's springs and switches
void Screen::scanScreenData(int roomIdx) {
    data_.springs.clear();
    data_.switches.clear();
    data_.pressureButtons.clear();
    
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
    
    // Scan switches and pressure buttons
    for (int y = 0; y < MAX_Y; ++y) {
        for (int x = 0; x < MAX_X; ++x) {
            Point p{x, y};
            wchar_t ch = getCharAt(p);
            if (Glyph::isSwitch(ch)) {
                bool isOn = (ch == Glyph::Switch_On);
                data_.switches.emplace_back(roomIdx, p, isOn);
            }
            else if (Glyph::isPressureButton(ch)) {
                data_.pressureButtons.emplace_back(roomIdx, p);
            }
        }
    }

    // Attach metadata targets to pressure buttons
    for (const auto& meta : metadata_.pressureButtons) {
        PressureButton* pb = PressureButton::findAt(*this, meta.position);
        if (!pb) {
            data_.pressureButtons.emplace_back(roomIdx, meta.position);
            pb = &data_.pressureButtons.back();
        }
        pb->setTargets(meta.clearTargets, *this);
    }
}

// Static method: Scan ALL data for all screens
void Screen::scanAllScreens(std::vector<Screen>& world, 
                             const RoomConnections& roomConnections,
                             std::map<RiddleKey, Riddle*>& riddlesByPosition,
                             Legend& legend) {
    
    // 1. Scan springs and switches in each screen
    for (size_t room = 0; room < world.size(); ++room) {
        world[room].scanScreenData((int)room);
    }
    
    // 2. Scan special doors (global configuration)
    SpecialDoor::scanAndPopulate(world);
    
    // 3. Scan obstacles (BFS across rooms)
    Obstacle::scanAllObstacles(world, roomConnections);
    
    // 4. Scan riddles (global configuration)
    Riddle::scanAllRiddles(riddlesByPosition);
    
    // 5. Scan legends (find 'L' in each screen)
    Legend::scanAllLegends(world, legend);
}

// Check if a point is in an unlit dark zone
bool Screen::isInDarkZone(const Point& p) const {
    for (const auto& zone : data_.darkZones) {
        if (!zone.isLit && zone.contains(p)) {
            return true;
        }
    }
    return false;
}

// Light up the dark zone containing the given point
void Screen::lightDarkZone(const Point& p) {
    for (auto& zone : data_.darkZones) {
        if (zone.contains(p)) {
            zone.isLit = true;
        }
    }
}

// Render the message box content from metadata
void Screen::renderMessageBox(const std::string& line1, const std::string& line2, const std::string& line3) {
    const MessageBoxMetadata& msg = metadata_.messageBox;
    if (!msg.hasMessage) return;
    
    Point anchor = msg.anchorPos;
    int boxWidth = msg.boxWidth;
    
    if (boxWidth <= 0) return;  // Could not determine box width
    
    // Helper lambda to center and render a line
    auto renderLine = [&](int row, const std::string& text) {
        // Clear the line first (fill with spaces)
        for (int i = 0; i < boxWidth; ++i) {
            setCharAt(Point{anchor.x + i, row}, L' ');
        }

        if (text.empty()) return;
        
        // Convert to wide string
        int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), (int)text.size(), nullptr, 0);
        std::wstring wtext(wlen, 0);
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), (int)text.size(), &wtext[0], wlen);
        
        // Truncate if too long
        if ((int)wtext.size() > boxWidth) {
            wtext = wtext.substr(0, boxWidth);
        }
        
        // Calculate padding for centering
        int padding = (boxWidth - (int)wtext.size()) / 2;
        
        // Write the centered text
        for (int i = 0; i < (int)wtext.size(); ++i) {
            setCharAt(Point{anchor.x + padding + i, row}, wtext[i]);
        }
    };
    
    // Render each line (anchor.y is line 1, anchor.y+1 is line 2, etc.)
    renderLine(anchor.y, line1);
    renderLine(anchor.y + 1, line2);
    renderLine(anchor.y + 2, line3);
}