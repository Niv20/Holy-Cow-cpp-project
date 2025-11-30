#include "Game.h"
#include "RiddleData.h"
#include "utils.h"
#include "Tiles.h"
#include <conio.h>
#include <windows.h>
#include <fstream>
#include <map>

using namespace std;

Game::Game() : visibleRoomIdx(0), isRunning(true), roomConnections(initRoomConnections()) { init(); }

void Game::init() {
    world = ScreenLoader::loadScreensFromFiles();
    if (world.empty()) { isRunning = false; return; }
    players.push_back(Player(Point(53, 18), "wdxase", Tiles::First_Player, 0));
    players.push_back(Player(Point(63, 18), "ilmjko", Tiles::Second_Player, 0));
    
    vector<RiddleData> riddles = initRiddles();
    for (auto& rd : riddles) {
        RiddleKey key{ rd.roomIdx, rd.position.x, rd.position.y };
        riddlesByPosition[key] = new Riddle(rd.riddle);
    }
    
    for (int room = 0; room < (int)world.size(); ++room) {
        Screen& s = world[room];
        for (int y = 0; y < Screen::MAX_Y; ++y) for (int x = 0; x < Screen::MAX_X; ++x) {
            Point p{ x, y }; if (s.getCharAt(p) == Tiles::Bomb) bombs.emplace_back(p, room, 5);
        }
    }
    legend.ensureRooms(world.size());
    for (int room = 0; room < (int)world.size(); ++room) legend.locateLegendForRoom(room, world[room]);
}

static const vector<string>& getRiddleTemplate() {
    static vector<string> cached; if (!cached.empty()) return cached;
    ifstream f("riddle.screen"); if (!f.is_open()) return cached;
    string line; while (getline(f, line)) cached.push_back(line);
    f.close(); if (!cached.empty()) { string& first = cached[0]; if (first.size() >= 3 && (unsigned char)first[0]==0xEF && (unsigned char)first[1]==0xBB && (unsigned char)first[2]==0xBF) first.erase(0,3); }
    return cached;
}

static const vector<string>& getPauseTemplate() {
    static vector<string> cached; if (!cached.empty()) return cached;
    ifstream f("Pause.screen"); if (!f.is_open()) return cached;
    string line; while (getline(f, line)) cached.push_back(line);
    f.close(); if (!cached.empty()) { string& first = cached[0]; if (first.size() >= 3 && (unsigned char)first[0]==0xEF && (unsigned char)first[1]==0xBB && (unsigned char)first[2]==0xBF) first.erase(0,3); }
    return cached;
}

void Game::run() {
    SetConsoleOutputCP(65001); setConsoleFont(); if (!isRunning) return;
    hideCursor(); world[visibleRoomIdx].draw(); refreshLegend(); drawPlayers();
    while (isRunning) { handleInput(); update(); Sleep(60); }
    for (auto& pair : riddlesByPosition) delete pair.second;
    cls();
}

void Game::refreshLegend() {
    char p1Inv = players.size() > 0 ? players[0].getCarried() : ' ';
    char p2Inv = players.size() > 1 ? players[1].getCarried() : ' ';
    legend.drawLegend(visibleRoomIdx, heartsCount, pointsCount, p1Inv, p2Inv);
}

void Game::drawPlayers() {
    for (const auto& p : players) if (p.getRoomIdx() == visibleRoomIdx) p.draw();
}

void Game::handleInput() {
    if (_kbhit()) {
        char key = _getch();
        if (key == ESC_KEY) { handlePause(); return; }
        for (auto& p : players) if (p.getRoomIdx() == visibleRoomIdx) p.handleKey(key);
    }
}

void Game::handlePause() {
    const vector<string>& pauseTemplate = getPauseTemplate();
    if (pauseTemplate.empty()) { isRunning = false; return; }
    
    Screen pauseScreen(pauseTemplate);
    cls();
    pauseScreen.draw();
    
    while (true) {
        if (_kbhit()) {
            char key = _getch();
            if (key == ESC_KEY) {
                // Continue the game - restore screen
                cls();
                world[visibleRoomIdx].draw();
                refreshLegend();
                drawPlayers();
                return;
            }
            else if (key == 'H' || key == 'h') {
                // Return to main menu
                isRunning = false;
                return;
            }
        }
        Sleep(60);
    }
}

void Game::update() {
    // Check if game is lost
    if (heartsCount <= 0) {
        isRunning = false;
        return;
    }
    
    for (auto& p : players) if (p.getRoomIdx() == visibleRoomIdx) {
        p.move(world[visibleRoomIdx]);
        Point pos = p.getPosition(); 
        wchar_t cell = world[visibleRoomIdx].getCharAt(pos);
        if (Tiles::isRiddle(cell)) handleRiddleEncounter(p);
    }
    
    checkAndProcessTransitions();
    tickAndHandleBombs();
    refreshLegend(); 
    drawPlayers();
}

void Game::handleRiddleEncounter(Player& player) {
    // Check if game is lost before showing riddle
    if (heartsCount <= 0) {
        isRunning = false;
        return;
    }
    
    int roomIdx = player.getRoomIdx();
    Point pos = player.getPosition();
    
    // Try to find riddle at exact position
    RiddleKey exactKey{ roomIdx, pos.x, pos.y };
    Riddle* riddle = nullptr;
    
    if (riddlesByPosition.find(exactKey) != riddlesByPosition.end()) {
        riddle = riddlesByPosition[exactKey];
    } else {
        // Fallback: find any riddle in this room
        for (auto& pair : riddlesByPosition) {
            if (pair.first.roomIdx == roomIdx) {
                riddle = pair.second;
                break;
            }
        }
    }
    
    if (!riddle) return;
    
    const vector<string>& templateScreen = getRiddleTemplate(); 
    if (templateScreen.empty()) return;
    
    vector<string> riddleScreenData = riddle->buildRiddleScreen(templateScreen); 
    Screen riddleScreen(riddleScreenData);
    cls(); riddleScreen.draw(); refreshLegend(); 
    
    char answer = '\0';
    while (true) {
        if (_kbhit()) {
            answer = _getch();
            if (answer == ESC_KEY) { 
                Point prevPos = pos; 
                if (pos.diff_x) prevPos.x -= pos.diff_x; 
                if (pos.diff_y) prevPos.y -= pos.diff_y; 
                player.setPosition(prevPos); 
                player.stop(); 
                break; 
            }
            else if (answer >= '1' && answer <= '4') {
                if (answer == riddle->getCorrectAnswer()) { 
                    pointsCount += riddle->getPoints(); 
                    world[roomIdx].setCharAt(pos, Tiles::Empty); 
                }
                else { 
                    riddle->halvePoints(); 
                    heartsCount--; 
                    Point prevPos = pos; 
                    if (pos.diff_x) prevPos.x -= pos.diff_x; 
                    if (pos.diff_y) prevPos.y -= pos.diff_y; 
                    player.setPosition(prevPos); 
                    player.stop(); 
                }
                break;
            }
        }
    }
    
    // Check if game is lost after answering riddle
    if (heartsCount <= 0) {
        isRunning = false;
        return;
    }
    
    cls(); world[visibleRoomIdx].draw(); refreshLegend(); drawPlayers();
}

void Game::checkAndProcessTransitions() {
    const int maxX = Screen::MAX_X;
    const int maxY = Screen::MAX_Y;

    struct TransitionInfo {
        Player* player;
        Direction direction;
        int targetRoom;
        Point originalPos; // includes diff_x/diff_y movement intent
        int order;
    };

    std::vector<TransitionInfo> transitions;
    int order = 0;

    // Collect transitions at exact edges
    for (auto& p : players) {
        if (p.getRoomIdx() != visibleRoomIdx) continue;
        Point pos = p.getPosition();
        Direction dir = Direction::None;
        if (pos.x <= 0) dir = Direction::Left;
        else if (pos.x >= maxX - 1) dir = Direction::Right;
        else if (pos.y <= 0) dir = Direction::Up;
        else if (pos.y >= maxY - 1) dir = Direction::Down;
        if (dir != Direction::None) {
            int nextRoom = roomConnections.getTargetRoom(visibleRoomIdx, dir);
            if (nextRoom != -1) transitions.push_back({ &p, dir, nextRoom, pos, order++ });
        }
    }

    if (transitions.empty()) return;

    int targetRoom = transitions[0].targetRoom;

    // Clear current edge cell visuals (players were visible at edge this frame)
    for (auto& t : transitions) world[visibleRoomIdx].refreshCell(t.originalPos);

    // Base spawn positions: opposite edge, preserve perpendicular coordinate
    std::vector<Point> spawns(transitions.size());
    for (size_t i = 0; i < transitions.size(); ++i) {
        const auto& t = transitions[i];
        Point s;
        switch (t.direction) {
            case Direction::Left:  s = Point(maxX - 2, t.originalPos.y); break;
            case Direction::Right: s = Point(1,        t.originalPos.y); break;
            case Direction::Up:    s = Point(t.originalPos.x, maxY - 2); break;
            case Direction::Down:  s = Point(t.originalPos.x, 1);        break;
            default: s = t.originalPos; break;
        }
        if (s.x < 1) s.x = 1; if (s.x > maxX - 2) s.x = maxX - 2;
        if (s.y < 1) s.y = 1; if (s.y > maxY - 2) s.y = maxY - 2;
        spawns[i] = s;
    }

    // If multiple players share same spawn, push the earlier entrant one step forward
    auto pushForwardOne = [&](size_t idx, const TransitionInfo& info) {
        int dx = info.originalPos.diff_x;
        int dy = info.originalPos.diff_y;
        if (dx == 0 && dy == 0) {
            // Derive from transition direction
            switch (info.direction) {
                case Direction::Left:  dx = -1; break;
                case Direction::Right: dx =  1; break;
                case Direction::Up:    dy = -1; break;
                case Direction::Down:  dy =  1; break;
                default: break;
            }
        }
        spawns[idx].x += dx; spawns[idx].y += dy;
        if (spawns[idx].x < 1) spawns[idx].x = 1; if (spawns[idx].x > maxX - 2) spawns[idx].x = maxX - 2;
        if (spawns[idx].y < 1) spawns[idx].y = 1; if (spawns[idx].y > maxY - 2) spawns[idx].y = maxY - 2;
    };

    // Single pass resolution: for each pair, enforce one-ahead rule
    for (size_t i = 0; i < spawns.size(); ++i) {
        for (size_t j = i + 1; j < spawns.size(); ++j) {
            if (spawns[i].x == spawns[j].x && spawns[i].y == spawns[j].y) {
                // i entered earlier than j -> i must be one step ahead
                pushForwardOne(i, transitions[i]);
            }
        }
    }

    // Apply final positions and preserve movement vectors
    for (size_t i = 0; i < transitions.size(); ++i) {
        Player* pl = transitions[i].player;
        Point newPos = spawns[i];
        newPos.diff_x = transitions[i].originalPos.diff_x;
        newPos.diff_y = transitions[i].originalPos.diff_y;
        pl->setRoomIdx(targetRoom);
        pl->setPosition(newPos);
    }

    // Switch camera if no one remains in current room
    bool cameraShouldMove = true;
    for (const auto& p : players) if (p.getRoomIdx() == visibleRoomIdx) { cameraShouldMove = false; break; }
    if (cameraShouldMove) { visibleRoomIdx = targetRoom; cls(); world[visibleRoomIdx].draw(); refreshLegend(); drawPlayers(); }
}

void Game::tickAndHandleBombs() { vector<Bomb> nextBombs; vector<Bomb> toExplode; nextBombs.reserve(bombs.size()); for (auto& b : bombs) if (b.tick()) toExplode.push_back(b); else nextBombs.push_back(b); bombs.swap(nextBombs); for (const auto& b : toExplode) explodeBomb(b); }

void Game::explodeBomb(const Bomb& b) { 
    int room = b.getRoomIdx(); Screen& s = world[room]; Point center = b.getPosition(); const int radius = 3; 
    int minX = max(0, center.x - radius), maxX = min(Screen::MAX_X - 1, center.x + radius); 
    int minY = max(0, center.y - radius), maxY = min(Screen::MAX_Y - 1, center.y + radius); 
    
    for (int y = minY; y <= maxY; ++y) 
        for (int x = minX; x <= maxX; ++x) { 
            Point p{ x, y }; wchar_t c = s.getCharAt(p); 
            if (c == Tiles::Bombable_Wall_H || c == Tiles::Bombable_Wall_V) 
                s.setCharAt(p, Tiles::Empty); 
        } 
    
    int hits = 0; 
    for (auto& pl : players) 
        if (pl.getRoomIdx() == room) { 
            Point pp = pl.getPosition(); 
            if (pp.x >= minX && pp.x <= maxX && pp.y >= minY && pp.y <= maxY) 
                ++hits; 
        } 
    
    heartsCount -= hits; 
    if (heartsCount < 0) heartsCount = 0; 
    
    // Check if game is lost after bomb explosion
    if (heartsCount <= 0) {
        isRunning = false;
    }
}

void Game::drawEverything() { cls(); world[visibleRoomIdx].draw(); refreshLegend(); drawPlayers(); }