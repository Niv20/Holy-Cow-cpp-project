#include "Game.h"
#include "RiddleData.h"
#include "utils.h"
#include "Glyph.h"
#include "Menu.h"
#include "SpecialDoorsData.h"
#include "Resources.h"
#include "DataStore.h"
#include <conio.h>
#include <windows.h>
#include <fstream>
#include <sstream>
#include <map>
#include <queue>
#include <set>
#include "RoomConnections.h"
#include "Player.h"
#include "Point.h"
#include "Riddle.h"
#include <vector>

using namespace std;

Game::Game() : visibleRoomIdx(0), isRunning(true), roomConnections(initRoomConnections()) { 
    initGame(); 
}

void Game::initGame() {
    // Use DataStore abstraction (currently delegates to Screen loader)
    world = DataStore::loadWorldScreens();
    if (world.empty()) { isRunning = false; return; }
    players.push_back(Player(Point(53, 18), "wdxase", Glyph::First_Player, 0));
    players.push_back(Player(Point(63, 18), "ilmjko", Glyph::Second_Player, 0));

    Screen::scanAllScreens(world);
    scanLegend();
    scanRiddles();
    scanObstacles();
    SpecialDoor::scanAndPopulate(world);
}


void Game::runApp() {

    bool exitProgram = false;

    while (!exitProgram) {

        MenuAction action = Menu::showStartMenu();
        switch (action) {

        case MenuAction::NewGame: {
            Game game; game.run();
            if (game.isGameLost()) { Menu::showLoseScreen(); }
            break;
        }

        case MenuAction::Instructions: {
            Menu::showInstructions();
            break;
        }

        case MenuAction::Exit: {
            exitProgram = true;
            break;
        }

        case MenuAction::Continue: break;
        case MenuAction::None: break;
        }
    }
}

void Game::scanLegend() {
    legend.ensureRooms(world.size());
    for (int room = 0; room < (int)world.size(); ++room) {
        legend.locateLegendForRoom(room, world[room]);
    }
}

void Game::scanRiddles() {
    vector<RiddleData> riddles = initRiddles();
    for (auto& rd : riddles) {
        RiddleKey key{ rd.roomIdx, rd.position.x, rd.position.y };
        riddlesByPosition[key] = new Riddle(rd.riddle);
    }
}

void Game::scanObstacles() {
    // Clear all screen obstacles first
    for (int room = 0; room < (int)world.size(); ++room) {
        world[room].getDataMutable().obstacles.clear();
    }
    
    // Build global visited per room
    vector<vector<vector<bool>>> visited(world.size(), vector<vector<bool>>(Screen::MAX_Y, vector<bool>(Screen::MAX_X,false)));
    for (int room = 0; room < (int)world.size(); ++room) {
        Screen& s = world[room];
        for (int y=0;y<Screen::MAX_Y;++y) for (int x=0;x<Screen::MAX_X;++x) {
            if (visited[room][y][x]) continue; Point start{ x,y }; wchar_t c = s.getCharAt(start);
            if (!Glyph::isObstacle(c)) continue;
            // BFS across rooms: neighbors include edges moving to connected rooms when crossing borders
            queue<pair<int,Point>> q; q.push({room,start}); visited[room][y][x]=true; vector<ObCell> component;
            auto enqueue = [&](int r, Point p) {
                if (r<0||r>=(int)world.size()) return; if (p.x<0||p.x>=Screen::MAX_X||p.y<0||p.y>=Screen::MAX_Y) return;
                if (visited[r][p.y][p.x]) return; if (!Glyph::isObstacle(world[r].getCharAt(p))) return; visited[r][p.y][p.x]=true; q.push({r,p}); };
            while(!q.empty()) {
                auto cur = q.front(); q.pop(); int cr = cur.first; Point cp = cur.second; component.push_back({cr,cp});
                const int dx[4]={1,-1,0,0}; const int dy[4]={0,0,1,-1};
                for (int i=0;i<4;++i) {
                    Point np{cp.x+dx[i], cp.y+dy[i]}; int nr = cr;
                    // If crossing border, move to connected room and wrap to opposite edge spawn
                    if (np.x < 0) { nr = roomConnections.getTargetRoom(cr, Direction::Left); np.x = Screen::MAX_X-1; }
                    else if (np.x >= Screen::MAX_X) { nr = roomConnections.getTargetRoom(cr, Direction::Right); np.x = 0; }
                    else if (np.y < 0) { nr = roomConnections.getTargetRoom(cr, Direction::Up); np.y = Screen::MAX_Y-1; }
                    else if (np.y >= Screen::MAX_Y) { nr = roomConnections.getTargetRoom(cr, Direction::Down); np.y = 0; }
                    if (nr == -1) continue; enqueue(nr, np);
                }
            }
            
            // Create the obstacle
            Obstacle obs(component);
            
            // Add this obstacle to all rooms it appears in
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

Obstacle* Game::findObstacleAt(int roomIdx, const Point& p) {
    auto& data = world[roomIdx].getDataMutable();
    for (auto& o : data.obstacles) {
        if (o.contains(roomIdx, p)) return &o;
    }
    return nullptr;
}

void Game::run() {

    if (!isRunning) return;

    SetConsoleOutputCP(65001); 
    setConsoleFont(); 
    hideCursor(); 

    world[visibleRoomIdx].draw(); 
    refreshLegend(); 
    drawPlayers();

    while (isRunning) { 
        handleInput(); 
        update(); 
        Sleep(180); 
    }
    
    if (heartsCount <= 0) {
        Menu::showLoseScreen();
    }
    
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
    const vector<string>& pauseTemplate = Resources::getPauseTemplate();
    if (pauseTemplate.empty()) { isRunning = false; return; }
    Screen pauseScreen(pauseTemplate);
    cls(); pauseScreen.draw();
    while (true) {
        if (_kbhit()) {
            char key = _getch();
            if (key == ESC_KEY) { cls(); world[visibleRoomIdx].draw(); refreshLegend(); drawPlayers(); return; }
            else if (key == 'H' || key == 'h') { isRunning = false; return; }
        }
        Sleep(180);
    }
}

void Game::update() {
    if (heartsCount <= 0) { isRunning = false; return; }
    for (auto& p : players) if (p.getRoomIdx() == visibleRoomIdx) {
        p.move(world[visibleRoomIdx], *this);
        Point pos = p.getPosition(); wchar_t cell = world[visibleRoomIdx].getCharAt(pos);
        if (Glyph::isRiddle(cell)) handleRiddleEncounter(p);
    }
    SpecialDoor::updateAll(*this);
    checkAndProcessTransitions(); tickAndHandleBombs(); refreshLegend(); drawPlayers();
}

void Game::handleRiddleEncounter(Player& player) {
    if (heartsCount <= 0) { isRunning = false; return; }
    int roomIdx = player.getRoomIdx();
    Point pos = player.getPosition();
    RiddleKey exactKey{ roomIdx, pos.x, pos.y };
    Riddle* riddle = nullptr;
    if (riddlesByPosition.find(exactKey) != riddlesByPosition.end()) {
        riddle = riddlesByPosition[exactKey];
    } else {
        for (auto& pair : riddlesByPosition) {
            if (pair.first.roomIdx == roomIdx) { riddle = pair.second; break; }
        }
    }
    if (!riddle) return;

    const vector<string>& templateScreen = Resources::getRiddleTemplate(); 
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
                    world[roomIdx].setCharAt(pos, Glyph::Empty); 
                }
                else { 
                    riddle-> halvePoints(); 
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

    if (heartsCount <= 0) { isRunning = false; return; }
    
    cls(); world[visibleRoomIdx].draw(); refreshLegend(); drawPlayers();
}

void Game::checkAndProcessTransitions() {
    const int maxX = Screen::MAX_X;
    const int maxY = Screen::MAX_Y;

    struct TransitionInfo {
        Player* player;
        Direction direction;
        int targetRoom;
        Point originalPos;
        int order;
    };

    std::vector<TransitionInfo> transitions;
    int order = 0;

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

    for (auto& t : transitions) world[visibleRoomIdx].refreshCell(t.originalPos);

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

    auto pushForwardOne = [&](size_t idx, const TransitionInfo& info) {
        int dx = info.originalPos.diff_x;
        int dy = info.originalPos.diff_y;
        if (dx == 0 && dy == 0) {
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

    for (size_t i = 0; i < spawns.size(); ++i) {
        for (size_t j = i + 1; j < spawns.size(); ++j) {
            if (spawns[i].x == spawns[j].x && spawns[i].y == spawns[j].y) {
                pushForwardOne(i, transitions[i]);
            }
        }
    }

    for (size_t i = 0; i < transitions.size(); ++i) {
        Player* pl = transitions[i].player;
        Point newPos = spawns[i];
        newPos.diff_x = transitions[i].originalPos.diff_x;
        newPos.diff_y = transitions[i].originalPos.diff_y;
        pl->setRoomIdx(targetRoom);
        pl->setPosition(newPos);
    }

    bool cameraShouldMove = true;
    for (const auto& p : players) if (p.getRoomIdx() == visibleRoomIdx) { cameraShouldMove = false; break; }
    if (cameraShouldMove) { visibleRoomIdx = targetRoom; cls(); world[visibleRoomIdx].draw(); refreshLegend(); drawPlayers(); }
}

void Game::tickAndHandleBombs() { 
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
        b.explode(*this);
    }
}

void Game::placeBomb(int roomIdx, const Point& pos, int delay) {
    bombs.emplace_back(pos, roomIdx, delay + 1);
}

void Game::drawEverything() { cls(); world[visibleRoomIdx].draw(); refreshLegend(); drawPlayers(); }

SpringData* Game::findSpringAt(int roomIdx, const Point& p) {
    auto& data = world[roomIdx].getDataMutable();
    for (auto& sp : data.springs) {
        if (sp.findCellIndex(p) != -1) {
            return &sp;
        }
    }
    return nullptr;
}

SwitchData* Game::findSwitchAt(int roomIdx, const Point& p) {
    auto& data = world[roomIdx].getDataMutable();
    for (auto& sw : data.switches) {
        if (sw.pos.x == p.x && sw.pos.y == p.y) {
            return &sw;
        }
    }
    return nullptr;
}

SpecialDoor* Game::findSpecialDoorAt(int roomIdx, const Point& p) {
    auto& dataDoors = world[roomIdx].getDataMutable().doors;
    for (auto& door : dataDoors) {
        if (door.position.x == p.x && door.position.y == p.y) {
            return &door;
        }
    }
    return nullptr;
}