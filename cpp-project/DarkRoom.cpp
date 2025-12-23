#include "DarkRoom.h"
#include "Screen.h"
#include "Player.h"
#include "Glyph.h"
#include "ScreenMetadata.h"
#define NOMINMAX
#include <windows.h>
#include <cmath>
#include <algorithm>
#include <set>
#include <map>

// Ensure Windows min/max macros do not clash with std::min/std::max if windows.h was pulled in earlier
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

// Check if a point is in any dark zone of the given screen
bool DarkRoomManager::isInDarkZone(const Screen& screen, const Point& p) {
    const auto& darkZones = screen.getData().getDarkZones();
    for (const auto& zone : darkZones) {
        if (zone.contains(p)) {
            return true;
        }
    }
    return false;
}

// Check if the room has any dark zones
bool DarkRoomManager::roomHasDarkness(const Screen& screen) {
    return !screen.getData().getDarkZones().empty();
}

// Check if torch is available (held by any player OR exists anywhere in the screen)
bool DarkRoomManager::isTorchAvailable(const Screen& screen, const std::vector<Player>& players, int roomIdx) {
    // Check if any player in this room is holding a torch
    for (const auto& player : players) {
        if (player.getRoomIdx() == roomIdx && player.getCarried() == '!') {
            return true;
        }
    }
    
    
    // Check if there's a torch anywhere in this screen
    for (int y = 0; y < Screen::MAX_Y; ++y) {
        for (int x = 0; x < Screen::MAX_X; ++x) {
            Point p{x, y};
            if (Glyph::isTorch(screen.getCharAt(p))) {
                return true;
            }
        }
    }
    
    
    return false;
}

// Update the info message area for dark rooms
void DarkRoomManager::getDarkRoomMessage(const Screen& screen, const std::vector<Player>& players, int roomIdx, std::string& line1, std::string& line2, std::string& line3) {
    const ScreenMetadata& meta = screen.getMetadata();

    // Default to original messages
    line1 = meta.getMessageBox().getLine1();
    line2 = meta.getMessageBox().getLine2();
    line3 = meta.getMessageBox().getLine3();

    // Only apply to rooms with dark zones
    if (!roomHasDarkness(screen)) return;
    
    // Check if room has a message box defined
    if (!meta.getMessageBox().getHasMessage()) return;
    
    bool hasTorch = isTorchAvailable(screen, players, roomIdx);
    
    if (!hasTorch) {
        // No torch available - override with warning message
        line1 = "";
        line2 = "Dark maze ahead. Carry a torch (!) to enter...";
        line3 = "";
    }
}





// Calculate distance between two points (adjusted for wider horizontal light)
// Horizontal distance is scaled down to make light wider horizontally
int DarkRoomManager::calculateDistance(const Point& a, const Point& b) {
    int dx = std::abs(a.getX() - b.getX());
    int dy = std::abs(a.getY() - b.getY());
    // Scale horizontal distance by 0.75 (dx * 3 / 4) to make light ~33% wider horizontally
    int scaledDx = (dx * 3) / 4;
    return (scaledDx > dy) ? scaledDx : dy;
}

// Get the darkness character for a given darkness level
wchar_t DarkRoomManager::getDarknessChar(int level) {
    switch (level) {
        case 0: return 0;
        case 1: return Glyph::Dark_Light;   // ░
        case 2: return Glyph::Dark_Medium;  // ▒
        case 3: return Glyph::Dark_Heavy;   // ▓
        default: return Glyph::Dark_Full;   // █
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

// Fixed 8x8 window (radius 4) around the queried cell to keep the scan tiny.
const int searchRadius = 4;
int minY = std::max(0, pos.getY() - searchRadius);
int maxY = std::min(Screen::MAX_Y - 1, pos.getY() + searchRadius);
int minX = std::max(0, pos.getX() - searchRadius);
int maxX = std::min(Screen::MAX_X - 1, pos.getX() + searchRadius);

for (int y = minY; y <= maxY; ++y) {
    for (int x = minX; x <= maxX; ++x) {
        Point torchPos(x, y);
        if (Glyph::isTorch(screen.getCharAt(torchPos))) {
            int dist = calculateDistance(pos, torchPos);
                if (dist < minDist) {
                    minDist = dist;
                    if (minDist == 0) return 0; // can't do better
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
    return 4;                                     // █
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
            if (playerPos.getX() == pos.getX() && playerPos.getY() == pos.getY()) {
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
            auto posKey = std::make_pair(pos.getX(), pos.getY());
            
            // Check for overlapping players
            if (playerPositionSet.count(posKey)) {
                // Find and update the existing entry to show overlap symbol
                for (auto& ps : playerSymbols) {
                    if (ps.first.getX() == pos.getX() && ps.first.getY() == pos.getY()) {
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
            Point p(x, y);
            wchar_t ch;
            
            // Check if there's a player at this position
            bool isPlayerPos = false;
            for (const auto& ps : playerSymbols) {
                if (ps.first.getX() == x && ps.first.getY() == y) {
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
    if (p.getX() < 0 || p.getX() >= Screen::MAX_X || p.getY() < 0 || p.getY() >= Screen::MAX_Y) return;
    
    wchar_t ch;
    if (roomHasDarkness(screen)) {
        ch = getDisplayChar(screen, p, players, roomIdx);
    } else {
        ch = screen.getCharAt(p);
    }
    
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos{(SHORT)p.getX(), (SHORT)p.getY()};
    SetConsoleCursorPosition(hOut, pos);
    DWORD written;
    WriteConsoleW(hOut, &ch, 1, &written, nullptr);
}


// Update only the cells affected by player movement - much faster than full redraw
void DarkRoomManager::updateDarknessAroundPlayers(const Screen& screen, const std::vector<Player>& players,
                                                   int roomIdx, const std::vector<Point>& previousPositions,
                                                   const std::vector<Point>& extraLightSources) {
    if (!roomHasDarkness(screen)) return;
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Collect all cells that need updating (within light radius of current and previous positions)
    std::set<std::pair<int,int>> cellsToUpdate;
    
    // Light spreads wider horizontally because calculateDistance scales dx by 0.75.
    // Cover that extra reach so we clear lingering halo artifacts.
    const int updateRadius = (HEAVY_SHADE_RADIUS * 4) / 3 + 1; // 9 -> 13
    
    // Add cells around previous positions (to clear old light) – only within dark zones
    for (const auto& prevPos : previousPositions) {
        for (int dy = -updateRadius; dy <= updateRadius; ++dy) {
            for (int dx = -updateRadius; dx <= updateRadius; ++dx) {
                int nx = prevPos.getX() + dx;
                int ny = prevPos.getY() + dy;
                if (nx >= 0 && nx < Screen::MAX_X && ny >= 0 && ny < Screen::MAX_Y) {
                    Point q(nx, ny);
                    if (isInDarkZone(screen, q)) {
                        cellsToUpdate.insert({nx, ny});
                    }
                }
            }
        }
    }
    
    // Add cells around current player positions (to draw new light) – restrict to dark zones
    for (const auto& player : players) {
        if (player.getRoomIdx() != roomIdx) continue;
        Point pos = player.getPosition();
        for (int dy = -updateRadius; dy <= updateRadius; ++dy) {
            for (int dx = -updateRadius; dx <= updateRadius; ++dx) {
                int nx = pos.getX() + dx;
                int ny = pos.getY() + dy;
                if (nx >= 0 && nx < Screen::MAX_X && ny >= 0 && ny < Screen::MAX_Y) {
                    Point q(nx, ny);
                    if (isInDarkZone(screen, q)) {
                        cellsToUpdate.insert({nx, ny});
                    }
                }
            }
        }
        // Ensure the player position itself is always updated (even if not inside dark zone)
        cellsToUpdate.insert({pos.getX(), pos.getY()});
    }

    // Add cells around extra light sources (e.g., dropped torches) – restrict to dark zones
    for (const auto& src : extraLightSources) {
        for (int dy = -updateRadius; dy <= updateRadius; ++dy) {
            for (int dx = -updateRadius; dx <= updateRadius; ++dx) {
                int nx = src.getX() + dx;
                int ny = src.getY() + dy;
                if (nx >= 0 && nx < Screen::MAX_X && ny >= 0 && ny < Screen::MAX_Y) {
                    Point q(nx, ny);
                    if (isInDarkZone(screen, q)) {
                        cellsToUpdate.insert({nx, ny});
                    }
                }
            }
        }
        // Ensure the light source cell itself is updated
        cellsToUpdate.insert({src.getX(), src.getY()});
    }
    
    // Also explicitly add previous positions themselves to clear player ghost
    for (const auto& prevPos : previousPositions) {
        if (prevPos.getX() >= 0 && prevPos.getX() < Screen::MAX_X && 
            prevPos.getY() >= 0 && prevPos.getY() < Screen::MAX_Y) {
            cellsToUpdate.insert({prevPos.getX(), prevPos.getY()});
        }
    }
    
    // Build player position map for this room
    std::map<std::pair<int,int>, wchar_t> playerSymbolMap;
    for (const auto& player : players) {
        if (player.getRoomIdx() == roomIdx) {
            Point pos = player.getPosition();
            auto key = std::make_pair(pos.getX(), pos.getY());
            if (playerSymbolMap.count(key)) {
                playerSymbolMap[key] = L'O'; // Overlap
            } else {
                playerSymbolMap[key] = player.getSymbol();
            }
        }
    }
    
    // Update each affected cell
    for (const auto& cell : cellsToUpdate) {
        Point p(cell.first, cell.second);
        wchar_t ch;
        
        // Check if player is at this position
        auto it = playerSymbolMap.find(cell);
        if (it != playerSymbolMap.end()) {
            ch = it->second;
        } else if (isInDarkZone(screen, p)) {
            // In dark zone - apply darkness effect
            ch = getDisplayChar(screen, p, players, roomIdx);
        } else {
            // Outside dark zone - show original character
            ch = screen.getCharAt(p);
        }
        
        COORD pos{(SHORT)p.getX(), (SHORT)p.getY()};
        SetConsoleCursorPosition(hOut, pos);
        DWORD written;
        WriteConsoleW(hOut, &ch, 1, &written, nullptr);
    }
}
