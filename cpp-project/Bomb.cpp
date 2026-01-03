#include "Bomb.h"
#include "Game.h"
#include "Screen.h"
#include "Glyph.h"
#include "Obstacle.h"
#include "ScreenBuffer.h"
#include <algorithm>
#include <set>

// Written by AI
// Helper to avoid Windows.h min/max macro conflict
template<typename T>
inline T clamp_min(T a, T b) { return (a < b) ? b : a; }
template<typename T>
inline T clamp_max(T a, T b) { return (a > b) ? b : a; }

// Explode a bomb: destroy weak walls, obstacles, AND damage players
// Returns indices of players who were hit
std::vector<int> Bomb::explode(Game& game) {
    Screen& s = game.getScreen(roomIdx);
    const int radius = 3;
    std::vector<int> hitPlayers;

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

    // Find and destroy obstacles that have at least one cell in the blast radius
    std::set<Obstacle*> obstaclesToDestroy;
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            Point p(x, y);
            wchar_t c = s.getCharAt(p);
            if (Glyph::isObstacle(c)) {
                Obstacle* obs = Obstacle::findAt(s, roomIdx, p);
                if (obs) {
                    obstaclesToDestroy.insert(obs);
                }
            }
        }
    }

    // Destroy all affected obstacles (erase all their cells from all rooms)
    for (Obstacle* obs : obstaclesToDestroy) {
        for (const auto& cell : obs->getCells()) {
            Screen& cellScreen = game.getScreen(cell.getRoomIdx());
            cellScreen.setCharAt(cell.getPos(), Glyph::Empty);
            if (cell.getRoomIdx() == game.getVisibleRoomIdx()) {
                cellScreen.refreshCell(cell.getPos());
            }
        }
    }

    // Rescan obstacles if any were destroyed
    if (!obstaclesToDestroy.empty()) {
        game.rescanObstacles();
    }

    // Damage players: each player hit = 1 heart lost
    // Track which players were hit
    auto& allPlayers = game.getPlayersMutable();
    for (size_t i = 0; i < allPlayers.size(); ++i) {
        auto& pl = allPlayers[i];
        if (pl.getRoomIdx() == roomIdx) {
            Point pp = pl.getPosition();
            if (pp.getX() >= minX && pp.getX() <= maxX && pp.getY() >= minY && pp.getY() <= maxY) {
                hitPlayers.push_back(static_cast<int>(i));
            }
        }
    }

    game.reduceHearts(static_cast<int>(hitPlayers.size()));
    return hitPlayers;
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
        std::vector<int> hitPlayers = b.explode(game);
        // Record life lost for each hit player
        for (int playerIdx : hitPlayers) {
            game.recordLifeLost(playerIdx);
        }
    }
}

// Static: Place a new bomb
void Bomb::place(std::vector<Bomb>& bombs, int roomIdx, const Point& pos, int delay) {
    bombs.emplace_back(pos, roomIdx, delay + 1);
}