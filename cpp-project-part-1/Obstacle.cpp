#include "Obstacle.h"
#include "Screen.h"
#include "Glyph.h"
#include "Game.h"
#include "RoomConnections.h"
#include <queue>
#include <set>

// Helper: map dx,dy to Direction for room crossing
static Direction dirFromDelta(int dx, int dy) {
    if (dx < 0) return Direction::Left;
    if (dx > 0) return Direction::Right;
    if (dy < 0) return Direction::Up;
    if (dy > 0) return Direction::Down;
    return Direction::None;
}

bool Obstacle::canPush(int dx, int dy, int force, Game& game, int speed) const {
    // Require enough force equal to obstacle size
    if (force < size()) return false;

    // Build a quick lookup of current cells positions per room to allow self-overlap during move
    // (when translating rigidly, interior cells move into positions vacated by other cells of the same obstacle)
    auto isOwnCellAt = [&](int room, const Point& p) -> bool {
        for (const auto& c : cells) if (c.roomIdx == room && c.pos.x == p.x && c.pos.y == p.y) return true; return false;
    };

    // Check every cell's destination is valid for the entire speed distance
    for (auto& c : cells) {
        int nx = c.pos.x + dx * speed; // multiply by speed
        int ny = c.pos.y + dy * speed;
        int room = c.roomIdx;

        // Handle crossing room borders: obstacles can cross rooms
        // Note: multi-step may cross multiple rooms, but for simplicity we handle one crossing
        if (nx < 0 || nx >= Screen::MAX_X || ny < 0 || ny >= Screen::MAX_Y) {
            Direction d = dirFromDelta(dx, dy);
            int targetRoom = game.getTargetRoom(room, d);
            if (targetRoom == -1) return false; // no connection -> blocked
            // Wrap position to 3 cells inward from opposite edge (same as applyPush)
            if (nx < 0) nx = Screen::MAX_X - 4;
            else if (nx >= Screen::MAX_X) nx = 3;
            if (ny < 0) ny = Screen::MAX_Y - 4;
            else if (ny >= Screen::MAX_Y) ny = 3;
            room = targetRoom;
        }

        Screen& s = game.getScreen(room);
        Point np{ nx, ny };
        wchar_t ch = s.getCharAt(np);

        // Walls always block
        if (Glyph::isWall(ch) || Glyph::isDoor(ch)) return false;

        // If destination has another obstacle cell:
        // allow only if it belongs to THIS obstacle (self-overlap during rigid move)
        if (Glyph::isObstacle(ch)) {
            if (!isOwnCellAt(room, np)) return false; // other obstacle blocks
            // else it's our own current cell which will vacate; allowed
            continue;
        }

        // Otherwise require empty space
        if (ch != Glyph::Empty) return false;
    }

    return true;
}

void Obstacle::applyPush(int dx, int dy, Game& game, int speed) {
    // Refresh-aware erase of old cells
    for (auto& c : cells) {
        Screen& s = game.getScreen(c.roomIdx);
        s.setCharAt(c.pos, Glyph::Empty);
        s.refreshCell(c.pos); // update console immediately so obstacle doesn't appear "swallowed"
    }

    // Perform movement by speed steps, including room crossing
    for (auto& c : cells) {
        int nx = c.pos.x + dx * speed; // multiply by speed
        int ny = c.pos.y + dy * speed;
        int room = c.roomIdx;

        if (nx < 0 || nx >= Screen::MAX_X || ny < 0 || ny >= Screen::MAX_Y) {
            Direction d = dirFromDelta(dx, dy);
            int targetRoom = game.getTargetRoom(room, d);
            // Assuming canPush already verified targetRoom != -1
            // Spawn obstacle 3 cells inward from edge
            if (nx < 0) nx = Screen::MAX_X - 4; // 3 cells from right edge
            else if (nx >= Screen::MAX_X) nx = 3; // 3 cells from left edge
            if (ny < 0) ny = Screen::MAX_Y - 4; // 3 cells from bottom edge
            else if (ny >= Screen::MAX_Y) ny = 3; // 3 cells from top edge
            room = targetRoom;
        }

        c.pos.x = nx;
        c.pos.y = ny;
        c.roomIdx = room;
    }

    // Draw obstacle at new positions only if in visible room
    int visibleRoom = game.getVisibleRoomIdx();
    for (auto& c : cells) {
        Screen& s = game.getScreen(c.roomIdx);
        s.setCharAt(c.pos, Glyph::Obstacle);
        // Only refresh if this cell is in the currently visible room
        if (c.roomIdx == visibleRoom) {
            s.refreshCell(c.pos);
        }
        // If not visible, it will be drawn when room is switched
    }
}

// Written by AI
void Obstacle::scanAllObstacles(std::vector<Screen>& world, const RoomConnections& roomConnections) {
    using std::vector;
    using std::queue;
    using std::pair;
    using std::set;
    
    // Clear all obstacles first
    for (int room = 0; room < (int)world.size(); ++room) {
        world[room].getDataMutable().obstacles.clear();
    }

    vector<vector<vector<bool>>> visited(world.size(), 
        vector<vector<bool>>(Screen::MAX_Y, vector<bool>(Screen::MAX_X, false)));
    
    for (int room = 0; room < (int)world.size(); ++room) {
        Screen& s = world[room];
        for (int y = 0; y < Screen::MAX_Y; ++y) {
            for (int x = 0; x < Screen::MAX_X; ++x) {
                if (visited[room][y][x]) continue;
                
                Point start{ x, y };
                wchar_t c = s.getCharAt(start);
                if (!Glyph::isObstacle(c)) continue;
                
                queue<pair<int, Point>> q;
                q.push({ room, start });
                visited[room][y][x] = true;
                vector<ObCell> component;
                
                auto enqueue = [&](int r, Point p) {
                    if (r < 0 || r >= (int)world.size()) return;
                    if (p.x < 0 || p.x >= Screen::MAX_X || p.y < 0 || p.y >= Screen::MAX_Y) return;
                    if (visited[r][p.y][p.x]) return;
                    if (!Glyph::isObstacle(world[r].getCharAt(p))) return;
                    visited[r][p.y][p.x] = true;
                    q.push({ r, p });
                };
                
                while (!q.empty()) {
                    auto cur = q.front();
                    q.pop();
                    int cr = cur.first;
                    Point cp = cur.second;
                    component.push_back({ cr, cp });
                    
                    const int dx[4] = { 1, -1, 0, 0 };
                    const int dy[4] = { 0, 0, 1, -1 };
                    
                    for (int i = 0; i < 4; ++i) {
                        Point np{ cp.x + dx[i], cp.y + dy[i] };
                        int nr = cr;
                        
                        if (np.x < 0) {
                            nr = roomConnections.getTargetRoom(cr, Direction::Left);
                            np.x = Screen::MAX_X - 1;
                        }
                        else if (np.x >= Screen::MAX_X) {
                            nr = roomConnections.getTargetRoom(cr, Direction::Right);
                            np.x = 0;
                        }
                        else if (np.y < 0) {
                            nr = roomConnections.getTargetRoom(cr, Direction::Up);
                            np.y = Screen::MAX_Y - 1;
                        }
                        else if (np.y >= Screen::MAX_Y) {
                            nr = roomConnections.getTargetRoom(cr, Direction::Down);
                            np.y = 0;
                        }
                        
                        if (nr == -1) continue;
                        enqueue(nr, np);
                    }
                }

                Obstacle obs(component);

                set<int> roomsInvolved;
                for (const auto& cell : component) {
                    roomsInvolved.insert(cell.roomIdx);
                }
                for (int involvedRoom : roomsInvolved) {
                    world[involvedRoom].getDataMutable().obstacles.push_back(obs);
                }
            }
        }
    }
}

// Static: Find obstacle at position
Obstacle* Obstacle::findAt(Screen& screen, int roomIdx, const Point& p) {
    auto& data = screen.getDataMutable();
    for (auto& o : data.obstacles) {
        if (o.contains(roomIdx, p)) {
            return &o;
        }
    }
    return nullptr;
}