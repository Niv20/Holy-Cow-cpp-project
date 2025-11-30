#include "Obstacle.h"
#include "Screen.h"
#include "Glyph.h"
#include "Game.h"

bool Obstacle::canPush(int dx, int dy, int force, Game& game) const {
    if (force < size()) return false;
    // Check target cells free across rooms (no walls or other obstacles)
    for (auto& c : cells) {
        Point np{ c.pos.x + dx, c.pos.y + dy };
        Screen& s = game.getScreen(c.roomIdx);
        wchar_t ch = s.getCharAt(np);
        if (Glyph::isWall(ch) || Glyph::isObstacle(ch)) return false;
        // Also verify within bounds
        if (np.x < 0 || np.x >= Screen::MAX_X || np.y < 0 || np.y >= Screen::MAX_Y) return false;
    }
    return true;
}

void Obstacle::applyPush(int dx, int dy, Game& game) {
    // Erase old
    for (auto& c : cells) game.getScreen(c.roomIdx).setCharAt(c.pos, Glyph::Empty);
    // Move
    for (auto& c : cells) { c.pos.x += dx; c.pos.y += dy; }
    // Draw new
    for (auto& c : cells) game.getScreen(c.roomIdx).setCharAt(c.pos, Glyph::Obstacle);
}
