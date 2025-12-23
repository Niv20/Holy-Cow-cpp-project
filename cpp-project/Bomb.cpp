#include "Bomb.h"
#include "Game.h"
#include "Screen.h"
#include "Glyph.h"
#include "Obstacle.h"
#include <algorithm>
#include <set>

// Written by AI
// Helper to avoid Windows.h min/max macro conflict
template<typename T>
inline T clamp_min(T a, T b) { return (a < b) ? b : a; }
template<typename T>
inline T clamp_max(T a, T b) { return (a > b) ? b : a; }

// Explode a bomb: destroy weak walls AND damage players
void Bomb::explode(Game& game) {
    Screen& s = game.getScreen(roomIdx);
    const int radius = 3;

    // First, remove the bomb '@' from the screen
    s.setCharAt(position, Glyph::Empty);
    if (roomIdx == game.getVisibleRoomIdx()) {
        s.refreshCell(position);
    }

    // Calculate blast area (distance 3 in all directions, including diagonally)
    int minX = (std::max)(0, position.getX() - radius);
    int maxX = (std::min)(Screen::MAX_X - 1, position.getX() + radius);
    int minY = (std::max)(0, position.getY() - radius);
    int maxY = (std::min)(Screen::MAX_Y - 1, position.getY() + radius);

    // Destroy weak walls (^) in blast radius
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            Point p(x, y);
            wchar_t c = s.getCharAt(p);
            if (Glyph::isBombableWall(c)) {
                s.setCharAt(p, Glyph::Empty);
                if (roomIdx == game.getVisibleRoomIdx()) {
                    s.refreshCell(p);
                }
            }
        }
    }

    // NOTE: Obstacles are NOT affected by bombs anymore per requirement.
    // We intentionally skip any obstacle removal logic here.

    // Damage players: each player hit = 1 heart lost
    int hits = 0;
    for (auto& pl : game.getPlayersMutable()) {
        if (pl.getRoomIdx() == roomIdx) {
            Point pp = pl.getPosition();
            if (pp.getX() >= minX && pp.getX() <= maxX && pp.getY() >= minY && pp.getY() <= maxY) {
                ++hits;
            }
        }
    }

    game.reduceHearts(hits);
}

// Static: Tick all bombs and handle explosions
void Bomb::tickAndHandleAll(std::vector<Bomb>& bombs, Game& game) {
    std::vector<Bomb> nextBombs;
    std::vector<Bomb> toExplode;
    nextBombs.reserve(bombs.size());

    for (auto& b : bombs) {
        if (b.tick()) {
            toExplode.push_back(b);
        } else {
            nextBombs.push_back(b);
        }
    }

    bombs.swap(nextBombs);

    for (auto& b : toExplode) {
        b.explode(game);
    }
}

// Static: Place a new bomb
void Bomb::place(std::vector<Bomb>& bombs, int roomIdx, const Point& pos, int delay) {
    bombs.emplace_back(pos, roomIdx, delay + 1);
}