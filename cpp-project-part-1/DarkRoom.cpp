#include "DarkRoom.h"
#include "Screen.h"
#include "Player.h"
#include "Glyph.h"
#define NOMINMAX
#include <windows.h>
#include <cmath>
#include <algorithm>
#include <set>
#include <map>

// Check if a point is in any dark zone of the given screen
bool DarkRoomManager::isInDarkZone(const Screen& screen, const Point& p) {
    const auto& darkZones = screen.getData().darkZones;
    for (const auto& zone : darkZones) {
        if (zone.contains(p)) {
            return true;
        }
    }
    return false;
}

// Check if the room has any dark zones
bool DarkRoomManager::roomHasDarkness(const Screen& screen) {
    return !screen.getData().darkZones.empty();
}

// Calculate distance between two points (Chebyshev distance for circular-ish light)
int DarkRoomManager::calculateDistance(const Point& a, const Point& b) {
    int dx = std::abs(a.x - b.x);
    int dy = std::abs(a.y - b.y);
    return (dx > dy) ? dx : dy;  // Chebyshev distance gives octagonal light pattern
}

// Get the darkness character for a given darkness level
wchar_t DarkRoomManager::getDarknessChar(int level) {
    switch (level) {
        case 0: return 0;  // Full light - no overlay
        case 1: return Glyph::Dark_Light;   // ?
        case 2: return Glyph::Dark_Medium;  // ?
        case 3: return Glyph::Dark_Heavy;   // ?
        default: return Glyph::Dark_Full;   // ?
    }
}

// Check if any player in the room is holding a torch
bool DarkRoomManager::anyPlayerHoldsTorch(const std::vector<Player>& players, int roomIdx) {
    for (const auto& player : players) {
        if (player.getRoomIdx() == roomIdx && player.getCarried() == '!') {
            return true;
        }
    }
    return false;
}

// Find closest torch-holding player distance to a point
int DarkRoomManager::closestTorchDistance(const Point& pos, const std::vector<Player>& players, int roomIdx) {
    int minDist = 9999;
    for (const auto& player : players) {
        if (player.getRoomIdx() == roomIdx && player.getCarried() == '!') {
            int dist = calculateDistance(pos, player.getPosition());
            if (dist < minDist) {
                minDist = dist;
            }
        }
    }
    return minDist;
}

// Find closest dropped torch distance to a point
int DarkRoomManager::closestDroppedTorchDistance(const Point& pos, const Screen& screen) {
    int minDist = 9999;
    // Scan the screen for dropped torches
    for (int y = 0; y < Screen::MAX_Y; ++y) {
        for (int x = 0; x < Screen::MAX_X; ++x) {
            Point torchPos{x, y};
            if (Glyph::isTorch(screen.getCharAt(torchPos))) {
                int dist = calculateDistance(pos, torchPos);
                if (dist < minDist) {
                    minDist = dist;
                }
            }
        }
    }
    return minDist;
}

// Find minimum distance to any light source (held or dropped torch)
int DarkRoomManager::closestLightSourceDistance(const Point& pos, const std::vector<Player>& players, 
                                                 int roomIdx, const Screen& screen) {
    int heldDist = closestTorchDistance(pos, players, roomIdx);
    int droppedDist = closestDroppedTorchDistance(pos, screen);
    return (heldDist < droppedDist) ? heldDist : droppedDist;
}

// Get the darkness level at a position (considers both held and dropped torches)
int DarkRoomManager::getDarknessLevel(const Point& pos, const std::vector<Player>& players, int roomIdx) {
    // This version only considers held torches - use getDarknessLevelWithDropped for full check
    int dist = closestTorchDistance(pos, players, roomIdx);
    
    if (dist <= FULL_LIGHT_RADIUS) return 0;      // Full visibility
    if (dist == LIGHT_SHADE_RADIUS) return 1;     // ░
    if (dist == MEDIUM_SHADE_RADIUS) return 2;    // ▒
    if (dist == HEAVY_SHADE_RADIUS) return 3;     // ▓
    return 4;                                      // █
}

// Get darkness level considering all light sources
static int getDarknessLevelFromDistance(int dist) {
    if (dist <= DarkRoomManager::FULL_LIGHT_RADIUS) return 0;
    if (dist == DarkRoomManager::LIGHT_SHADE_RADIUS) return 1;
    if (dist == DarkRoomManager::MEDIUM_SHADE_RADIUS) return 2;
    if (dist == DarkRoomManager::HEAVY_SHADE_RADIUS) return 3;
    return 4;
}

// Check if player can move to target position
bool DarkRoomManager::canEnterPosition(const Screen& screen, const Player& player, const Point& target,
                                        const std::vector<Player>& allPlayers, int roomIdx) {
    // If target is not in a dark zone, always allow
    if (!isInDarkZone(screen, target)) {
        return true;
    }
    
    // If player has torch, can enter dark zone
    if (player.getCarried() == '!') {
        return true;
    }
    
    // Check if target is lit by any light source (held by other players OR dropped torches)
    int lightDist = closestLightSourceDistance(target, allPlayers, roomIdx, screen);
    if (lightDist <= FULL_LIGHT_RADIUS) {
        return true;  // Area is lit by another player's torch or dropped torch
    }
    
    // No torch and not in lit area, cannot enter dark zone
    return false;
}

// Get the character to display at a position
wchar_t DarkRoomManager::getDisplayChar(const Screen& screen, const Point& pos,
                                         const std::vector<Player>& players, int currentRoomIdx) {
    wchar_t originalChar = screen.getCharAt(pos);
    
    // If not in dark zone, return original
    if (!isInDarkZone(screen, pos)) {
        return originalChar;
    }
    
    // Always show player positions
    for (const auto& player : players) {
        if (player.getRoomIdx() == currentRoomIdx) {
            Point playerPos = player.getPosition();
            if (playerPos.x == pos.x && playerPos.y == pos.y) {
                return originalChar;  // Show player position
            }
        }
    }
    
    // Calculate distance to closest light source (held or dropped torch)
    int lightDist = closestLightSourceDistance(pos, players, currentRoomIdx, screen);
    int darkLevel = getDarknessLevelFromDistance(lightDist);
    
    if (darkLevel == 0) {
        return originalChar;  // Full visibility
    }
    
    // Return the darkness shade character
    return getDarknessChar(darkLevel);
}

// Draw the screen with darkness overlay (includes player drawing to prevent flicker)
void DarkRoomManager::drawWithDarkness(const Screen& screen, const std::vector<Player>& players, int roomIdx) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    
    bool hasDarkZones = roomHasDarkness(screen);
    
    // Build a map of player positions for this room
    std::vector<std::pair<Point, wchar_t>> playerSymbols;
    std::set<std::pair<int,int>> playerPositionSet;
    
    for (const auto& player : players) {
        if (player.getRoomIdx() == roomIdx) {
            Point pos = player.getPosition();
            auto posKey = std::make_pair(pos.x, pos.y);
            
            // Check for overlapping players
            if (playerPositionSet.count(posKey)) {
                // Find and update the existing entry to show overlap symbol
                for (auto& ps : playerSymbols) {
                    if (ps.first.x == pos.x && ps.first.y == pos.y) {
                        ps.second = L'O'; // Overlap symbol
                        break;
                    }
                }
            } else {
                playerSymbols.push_back({pos, player.getSymbol()});
                playerPositionSet.insert(posKey);
            }
        }
    }
    
    for (int y = 0; y < Screen::MAX_Y; ++y) {
        std::wstring out;
        out.reserve(Screen::MAX_X);
        
        for (int x = 0; x < Screen::MAX_X; ++x) {
            Point p{x, y};
            wchar_t ch;
            
            // Check if there's a player at this position
            bool isPlayerPos = false;
            for (const auto& ps : playerSymbols) {
                if (ps.first.x == x && ps.first.y == y) {
                    ch = ps.second;
                    isPlayerPos = true;
                    break;
                }
            }
            
            if (!isPlayerPos) {
                if (hasDarkZones) {
                    ch = getDisplayChar(screen, p, players, roomIdx);
                } else {
                    ch = screen.getCharAt(p);
                }
            }
            
            out.push_back(ch);
        }
        
        COORD linePos{0, (SHORT)y};
        SetConsoleCursorPosition(hOut, linePos);
        DWORD written;
        WriteConsoleW(hOut, out.c_str(), (DWORD)out.size(), &written, nullptr);
    }
}

// Refresh a single cell with darkness consideration
void DarkRoomManager::refreshCellWithDarkness(const Screen& screen, const Point& p,
                                               const std::vector<Player>& players, int roomIdx) {
    if (p.x < 0 || p.x >= Screen::MAX_X || p.y < 0 || p.y >= Screen::MAX_Y) return;
    
    wchar_t ch;
    if (roomHasDarkness(screen)) {
        ch = getDisplayChar(screen, p, players, roomIdx);
    } else {
        ch = screen.getCharAt(p);
    }
    
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos{(SHORT)p.x, (SHORT)p.y};
    SetConsoleCursorPosition(hOut, pos);
    DWORD written;
    WriteConsoleW(hOut, &ch, 1, &written, nullptr);
}

// Update only the cells affected by player movement - much faster than full redraw
void DarkRoomManager::updateDarknessAroundPlayers(const Screen& screen, const std::vector<Player>& players,
                                                   int roomIdx, const std::vector<Point>& previousPositions) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Collect all cells that need updating (within light radius of current and previous positions)
    std::set<std::pair<int,int>> cellsToUpdate;
    
    const int updateRadius = HEAVY_SHADE_RADIUS + 1; // Update slightly beyond visible light
    
    // Add cells around previous positions (to clear old light)
    for (const auto& prevPos : previousPositions) {
        for (int dy = -updateRadius; dy <= updateRadius; ++dy) {
            for (int dx = -updateRadius; dx <= updateRadius; ++dx) {
                int nx = prevPos.x + dx;
                int ny = prevPos.y + dy;
                if (nx >= 0 && nx < Screen::MAX_X && ny >= 0 && ny < Screen::MAX_Y) {
                    if (isInDarkZone(screen, Point{nx, ny})) {
                        cellsToUpdate.insert({nx, ny});
                    }
                }
            }
        }
    }
    
    // Add cells around current player positions (to draw new light)
    for (const auto& player : players) {
        if (player.getRoomIdx() != roomIdx) continue;
        Point pos = player.getPosition();
        for (int dy = -updateRadius; dy <= updateRadius; ++dy) {
            for (int dx = -updateRadius; dx <= updateRadius; ++dx) {
                int nx = pos.x + dx;
                int ny = pos.y + dy;
                if (nx >= 0 && nx < Screen::MAX_X && ny >= 0 && ny < Screen::MAX_Y) {
                    if (isInDarkZone(screen, Point{nx, ny})) {
                        cellsToUpdate.insert({nx, ny});
                    }
                }
            }
        }
    }
    
    // Build player position map for this room
    std::map<std::pair<int,int>, wchar_t> playerSymbolMap;
    for (const auto& player : players) {
        if (player.getRoomIdx() == roomIdx) {
            Point pos = player.getPosition();
            auto key = std::make_pair(pos.x, pos.y);
            if (playerSymbolMap.count(key)) {
                playerSymbolMap[key] = L'O'; // Overlap
            } else {
                playerSymbolMap[key] = player.getSymbol();
            }
        }
    }
    
    // Update each affected cell
    for (const auto& cell : cellsToUpdate) {
        Point p{cell.first, cell.second};
        wchar_t ch;
        
        // Check if player is at this position
        auto it = playerSymbolMap.find(cell);
        if (it != playerSymbolMap.end()) {
            ch = it->second;
        } else {
            ch = getDisplayChar(screen, p, players, roomIdx);
        }
        
        COORD pos{(SHORT)p.x, (SHORT)p.y};
        SetConsoleCursorPosition(hOut, pos);
        DWORD written;
        WriteConsoleW(hOut, &ch, 1, &written, nullptr);
    }
    
    // Always draw players (even outside dark zones)
    for (const auto& [posKey, symbol] : playerSymbolMap) {
        if (cellsToUpdate.find(posKey) == cellsToUpdate.end()) {
            // Player is outside dark zone, draw them directly
            COORD pos{(SHORT)posKey.first, (SHORT)posKey.second};
            SetConsoleCursorPosition(hOut, pos);
            DWORD written;
            WriteConsoleW(hOut, &symbol, 1, &written, nullptr);
        }
    }
}
