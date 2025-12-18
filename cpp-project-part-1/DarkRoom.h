#pragma once
#include "Point.h"
#include "ScreenMetadata.h"
#include <vector>

// Forward declarations
class Screen;
class Player;
class Game;

// DarkRoomManager - Handles darkness rendering and torch light effects
// 
// Dark zones are defined in screen metadata as rectangular areas.
// When a dark zone exists:
// - The area is rendered as full darkness (?) by default
// - Players cannot enter dark zones without holding a torch (!)
// - When a player holds a torch, light radiates from them in a circle:
//   - Distance 0-2: Full visibility (original characters)
//   - Distance 3: Light shade ?
//   - Distance 4: Medium shade ?
//   - Distance 5: Heavy shade ?
//   - Distance 6+: Full darkness ?
// - Dropping the torch makes everything dark except player and torch positions

class DarkRoomManager {
public:
    // Light radius constants
    static constexpr int FULL_LIGHT_RADIUS = 2;    // Full visibility within this radius
    static constexpr int LIGHT_SHADE_RADIUS = 3;   // ? at this distance
    static constexpr int MEDIUM_SHADE_RADIUS = 4;  // ? at this distance
    static constexpr int HEAVY_SHADE_RADIUS = 5;   // ? at this distance
    // Beyond heavy shade radius = full darkness ?
    
    // Check if a point is in any dark zone of the given screen
    static bool isInDarkZone(const Screen& screen, const Point& p);
    
    // Check if player can move to target position (considering darkness and torch)
    // Now also considers light from other players' torches
    static bool canEnterPosition(const Screen& screen, const Player& player, const Point& target,
                                  const std::vector<Player>& allPlayers, int roomIdx);
    
    // Get the character to display at a position, considering darkness and torch light
    // Returns the original character if visible, or a darkness shade character
    static wchar_t getDisplayChar(const Screen& screen, const Point& pos, 
                                   const std::vector<Player>& players, int currentRoomIdx);
    
    // Draw the screen with darkness overlay (includes player drawing to prevent flicker)
    static void drawWithDarkness(const Screen& screen, const std::vector<Player>& players, int roomIdx);
    
    // Update only the cells affected by player movement (much faster than full redraw)
    // Call this every frame instead of drawWithDarkness for smooth performance
    static void updateDarknessAroundPlayers(const Screen& screen, const std::vector<Player>& players, 
                                             int roomIdx, const std::vector<Point>& previousPositions);
    
    // Refresh a single cell with darkness consideration
    static void refreshCellWithDarkness(const Screen& screen, const Point& p,
                                         const std::vector<Player>& players, int roomIdx);
    
    // Check if the room has any dark zones
    static bool roomHasDarkness(const Screen& screen);
    
    // Get the darkness level at a position based on distance from torch-holding players
    // Returns 0 = full light, 1 = light shade, 2 = medium shade, 3 = heavy shade, 4 = full dark
    static int getDarknessLevel(const Point& pos, const std::vector<Player>& players, int roomIdx);
    
private:
    // Calculate distance between two points (using maximum of dx and dy for circular-ish light)
    static int calculateDistance(const Point& a, const Point& b);
    
    // Get the darkness character for a given darkness level
    static wchar_t getDarknessChar(int level);
    
    // Check if any player in the room is holding a torch
    static bool anyPlayerHoldsTorch(const std::vector<Player>& players, int roomIdx);
    
    // Find closest torch-holding player distance to a point
    static int closestTorchDistance(const Point& pos, const std::vector<Player>& players, int roomIdx);
    
    // Find closest dropped torch distance to a point
    static int closestDroppedTorchDistance(const Point& pos, const Screen& screen);
    
    // Find minimum distance to any light source (held or dropped torch)
    static int closestLightSourceDistance(const Point& pos, const std::vector<Player>& players, 
                                           int roomIdx, const Screen& screen);
};
