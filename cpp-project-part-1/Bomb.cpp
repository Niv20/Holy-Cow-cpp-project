#include "Bomb.h"
#include "Game.h"
#include "Screen.h"
#include "Glyph.h"
#include "Obstacle.h"
#include <algorithm>
#include <set>

// Helper to avoid Windows.h min/max macro conflict
template<typename T>
inline T clamp_min(T a, T b) { return (a < b) ? b : a; }

template<typename T>
inline T clamp_max(T a, T b) { return (a > b) ? b : a; }

// Explode a bomb: destroy weak walls, destroy obstacles if any cell hit, damage players
void Bomb::explode(Game& game) {
    Screen& s = game.getScreen(roomIdx);
    const int radius = 3;
    
    // First, remove the bomb '@' from the screen
    s.setCharAt(position, Glyph::Empty);
    if (roomIdx == game.getVisibleRoomIdx()) {
        s.refreshCell(position);
    }
    
    // Calculate blast area (distance 3 in all directions, including diagonally)
    int minX = clamp_min(0, position.x - radius);
    int maxX = clamp_max(Screen::MAX_X - 1, position.x + radius);
    int minY = clamp_min(0, position.y - radius);
    int maxY = clamp_max(Screen::MAX_Y - 1, position.y + radius);
    
    // 1. Destroy weak walls (~, -, |) in blast radius
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            Point p{ x, y };
            wchar_t c = s.getCharAt(p);
            if (Glyph::isBombableWall(c)) {
                s.setCharAt(p, Glyph::Empty);
                if (roomIdx == game.getVisibleRoomIdx()) {
                    s.refreshCell(p);
                }
            }
        }
    }
    
    // 2. Destroy obstacles: if ANY cell of obstacle is in blast radius, destroy ENTIRE obstacle
    // We need to check the obstacles in this room and mark which ones to remove
    auto& roomData = game.getScreen(roomIdx).getDataMutable();
    std::set<Obstacle*> obstaclesToRemove;
    
    for (auto& obs : roomData.obstacles) {
        bool hitByBlast = false;
        for (const auto& cell : obs.getCells()) {
            if (cell.roomIdx == roomIdx) {
                Point cp = cell.pos;
                if (cp.x >= minX && cp.x <= maxX && cp.y >= minY && cp.y <= maxY) {
                    hitByBlast = true;
                    break;
                }
            }
        }
        if (hitByBlast) {
            obstaclesToRemove.insert(&obs);
        }
    }
    
    // Remove hit obstacles: erase all cells from ALL screens (obstacle might span rooms)
    for (auto* obs : obstaclesToRemove) {
        for (const auto& cell : obs->getCells()) {
            Screen& sc = game.getScreen(cell.roomIdx);
            sc.setCharAt(cell.pos, Glyph::Empty);
            if (cell.roomIdx == game.getVisibleRoomIdx()) {
                sc.refreshCell(cell.pos);
            }
            
            // Also remove this obstacle from the target room's data
            auto& targetRoomData = game.getScreen(cell.roomIdx).getDataMutable();
            targetRoomData.obstacles.erase(
                std::remove_if(targetRoomData.obstacles.begin(), targetRoomData.obstacles.end(),
                    [obs](const Obstacle& o) { 
                        return &o == obs; 
                    }),
                targetRoomData.obstacles.end()
            );
        }
    }
    
    // 3. Damage players: each player hit = 1 heart lost
    int hits = 0;
    for (auto& pl : game.getPlayersMutable()) {
        if (pl.getRoomIdx() == roomIdx) {
            Point pp = pl.getPosition();
            if (pp.x >= minX && pp.x <= maxX && pp.y >= minY && pp.y <= maxY) {
                ++hits;
            }
        }
    }
    
    game.reduceHearts(hits);
}
