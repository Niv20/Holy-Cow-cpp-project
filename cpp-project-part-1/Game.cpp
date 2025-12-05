#include <conio.h>
#include <windows.h>
#include <queue>
#include <set>
#include <vector>
#include <string>
#include <utility>

#include "Game.h"
#include "utils.h"
#include "Glyph.h"
#include "Menu.h"
#include "RoomConnections.h"
#include "Player.h"
#include "Point.h"
#include "Riddle.h"
#include "Obstacle.h"

using std::vector;
using std::string;
using std::queue;
using std::pair;
using std::set;


/*      (__)
'\------(oo)    Constructor
  ||    (__)
  ||w--||                */

Game::Game() : visibleRoomIdx(0), isRunning(true), roomConnections(initRoomConnections()) { 
    initGame(); 
}

void Game::initGame() {

    world = Screen::loadScreensFromFiles();
 
    if (world.empty()) { 
        isRunning = false; 
        return; 
    }

    Screen::scanAllScreens(world, roomConnections, riddlesByPosition, legend);

    players.push_back(Player(Point(53, 18), "wdxase", Glyph::First_Player, 0));
    players.push_back(Player(Point(63, 18), "ilmjko", Glyph::Second_Player, 0));

    // Initialize final room tracking for both players
    playerReachedFinalRoom.resize(players.size(), false);
    finalRoomFocusTicks = 0;
}

/*      (__)
'\------(oo)    Start the application
  ||    (__)
  ||w--||                           */

void Game::runApp() {

// Initialize console settings once at the start of the application
SetConsoleOutputCP(65001); 
setConsoleFont(); 
hideCursor();

bool exitProgram = false;

while (!exitProgram) {

    MenuAction action = Menu::showStartMenu();
        switch (action) {

            case MenuAction::NewGame: {
                Game game; game.start();
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

void Game::start() {

if (!isRunning) return;

world[visibleRoomIdx].draw(); 
refreshLegend(); 
drawPlayers();

    while (isRunning) { 
        handleInput();
        if (!isRunning) break;
        update(); 
        Sleep(GAME_TICK_DELAY_MS); 
    }
    
    cls();
    
    if (heartsCount <= 0) {
        Menu::showLoseScreen();
    }
    else {
        // Check if both players reached final room (win condition)
        bool allPlayersWon = true;
        for (bool reached : playerReachedFinalRoom) {
            if (!reached) {
                allPlayersWon = false;
                break;
            }
        }
        
        if (allPlayersWon) {
            Menu::showWinScreen();
        }
    }
    
    for (auto& pair : riddlesByPosition) delete pair.second;
}


void Game::handlePause() {

    const vector<string>& pauseTemplate = Menu::getPauseTemplate();

    if (pauseTemplate.empty()) {
        isRunning = false;
        return;
    }

    Screen pauseScreen(pauseTemplate);

    cls();
    pauseScreen.draw();

    while (true) {

        if (_kbhit()) {

            char key = _getch();

            if (key == ESC_KEY) {
                cls();
                world[visibleRoomIdx].draw();
                refreshLegend();
                drawPlayers();
                return;
            }
            else if (key == 'H' || key == 'h') {
                cls();
                isRunning = false;
                return;
            }
        }

        Sleep(GAME_TICK_DELAY_MS);
    }
}


void Game::refreshLegend() {
    char p1Inv = players.size() > 0 ? players[0].getCarried() : ' ';
    char p2Inv = players.size() > 1 ? players[1].getCarried() : ' ';
    legend.drawLegend(visibleRoomIdx, heartsCount, pointsCount, p1Inv, p2Inv);
}

void Game::drawPlayers() {
    for (const auto& p : players) {
        if (p.getRoomIdx() == visibleRoomIdx)
            p.draw();
    }
}

void Game::handleInput() {

    if (_kbhit()) {

        char key = _getch();

        if (key == ESC_KEY) { 
            handlePause(); 
            return; 
        }

        // If both players reached final room, any key returns to start menu
        bool allAtFinal = true;
        for (size_t i = 0; i < players.size(); ++i) {
            if (players[i].getRoomIdx() != FINAL_ROOM_INDEX) { allAtFinal = false; break; }
        }
        if (allAtFinal) {
            // End game loop immediately
            isRunning = false;
            return;
        }

        // Normal input: prevent moving player who reached final room
        for (size_t i = 0; i < players.size(); ++i) {
            auto& p = players[i];
            if (p.getRoomIdx() == visibleRoomIdx && !hasPlayerReachedFinalRoom(i))
                p.handleKey(key);
        }
    }
}

void Game::update() {

    if (heartsCount <= 0) { 
        isRunning = false; 
        return; 
    }

    // Move all players
    for (size_t i = 0; i < players.size(); ++i) {
        auto& p = players[i];

        if (p.getRoomIdx() == visibleRoomIdx) {
            p.move(world[visibleRoomIdx], *this);
            Point pos = p.getPosition();
            wchar_t cell = world[visibleRoomIdx].getCharAt(pos);
            if (Glyph::isRiddle(cell))
                Riddle::handleEncounter(p, riddlesByPosition, *this);
        }
    }
    
    SpecialDoor::updateAll(*this);
    checkAndProcessTransitions(); 
    
    // Update final room tracking AFTER transitions
    for (size_t i = 0; i < players.size(); ++i) {
        if (players[i].getRoomIdx() == FINAL_ROOM_INDEX) {
            if (!playerReachedFinalRoom[i]) {
                playerReachedFinalRoom[i] = true;
                
                // Player just entered final room
                // Focus on final room for a brief period
                visibleRoomIdx = FINAL_ROOM_INDEX;
                finalRoomFocusTicks = FINAL_ROOM_FOCUS_TICKS;
                cls();
                world[visibleRoomIdx].draw();
                refreshLegend();
                drawPlayers();
            }
        }
    }
    
    // If focusing on final room, count down and then switch to other player if exists
    if (finalRoomFocusTicks > 0) {
        finalRoomFocusTicks--;
        if (finalRoomFocusTicks == 0) {
            // Switch camera to the room where any non-final player is
            for (size_t j = 0; j < players.size(); ++j) {
                if (players[j].getRoomIdx() != FINAL_ROOM_INDEX) {
                    visibleRoomIdx = players[j].getRoomIdx();
                    cls();
                    world[visibleRoomIdx].draw();
                    refreshLegend();
                    drawPlayers();
                    break;
                }
            }
        }
    }
    
    // Check if both players reached final room - any key will now exit to start
    bool allPlayersReachedFinal = true;
    for (size_t i = 0; i < players.size(); ++i) {
        if (players[i].getRoomIdx() != FINAL_ROOM_INDEX) {
            allPlayersReachedFinal = false;
            break;
        }
    }
    
    if (allPlayersReachedFinal) {
        // Stop all updates; handleInput will catch key and exit
        Bomb::tickAndHandleAll(bombs, *this);
        refreshLegend(); 
        drawPlayers();
        return;
    }
    
    Bomb::tickAndHandleAll(bombs, *this);
    refreshLegend(); 
    drawPlayers();
}

// Written by AI !!!
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

    vector<TransitionInfo> transitions;
    int order = 0;

    // Detect transitions when crossing an edge
    for (auto& p : players) {
        if (p.getRoomIdx() != visibleRoomIdx) continue;
        Point pos = p.getPosition();
        Direction dir = Direction::None;
        if (pos.x <= 0)
            dir = Direction::Left;
        else if (pos.x >= maxX - 1)
            dir = Direction::Right;
        else if (pos.y <= 0)
            dir = Direction::Up;
        else if (pos.y >= maxY - 1)
            dir = Direction::Down;

        if (dir != Direction::None) {
            int nextRoom = roomConnections.getTargetRoom(visibleRoomIdx, dir);
            if (nextRoom != -1) transitions.push_back({ &p, dir, nextRoom, pos, order++ });
        }
    }

    if (transitions.empty())
        return;

    // Erase players from current room at their original positions to prevent wrap ghosting
    for (auto& t : transitions)
        world[visibleRoomIdx].refreshCell(t.originalPos);

    // Compute spawn positions per player based on their direction and target room
    vector<Point> spawns(transitions.size());

    for (size_t i = 0; i < transitions.size(); ++i) {
        const auto& t = transitions[i];
        Point s;
        switch (t.direction) {
            case Direction::Left:  s = Point(maxX - 2, t.originalPos.y); break;
            case Direction::Right: s = Point(1,        t.originalPos.y); break;
            case Direction::Up:    s = Point(t.originalPos.x, maxY - 2); break;
            case Direction::Down:  s = Point(t.originalPos.x, 1);        break;
            default:               s = t.originalPos; break;
        }
        if (s.x < 1) s.x = 1; if (s.x > maxX - 2) s.x = maxX - 2;
        if (s.y < 1) s.y = 1; if (s.y > maxY - 2) s.y = maxY - 2;
        spawns[i] = s;
    }

    // Spread colliding spawns based on direction
    for (size_t i = 0; i < transitions.size(); ++i) {
        for (size_t j = i + 1; j < spawns.size(); ++j) {
            if (spawns[i].x == spawns[j].x && spawns[i].y == spawns[j].y) {
                Direction dir = transitions[i].direction;
                if (dir == Direction::Left || dir == Direction::Right) {
                    spawns[j].y += 1;
                    if (spawns[j].y > maxY - 2) spawns[j].y = maxY - 2;
                } else if (dir == Direction::Up || dir == Direction::Down) {
                    spawns[j].x += 1;
                    if (spawns[j].x > maxX - 2) spawns[j].x = maxX - 2;
                }
            }
        }
    }

    // Apply transitions per player
    for (size_t i = 0; i < transitions.size(); ++i) {
        Player* pl = transitions[i].player;
        Point newPos = spawns[i];
        newPos.diff_x = transitions[i].originalPos.diff_x;
        newPos.diff_y = transitions[i].originalPos.diff_y;
        pl->setRoomIdx(transitions[i].targetRoom);
        pl->setPosition(newPos);
    }

    // Camera logic:
    bool anyPlayerRemainsInCurrentRoom = false;
    for (const auto& p : players) {
        if (p.getRoomIdx() == visibleRoomIdx) {
            anyPlayerRemainsInCurrentRoom = true;
            break;
        }
    }
    if (anyPlayerRemainsInCurrentRoom) {
        return;
    }

    int focusRoom = transitions.back().targetRoom;
    visibleRoomIdx = focusRoom;
    drawEverything();
}

void Game::drawEverything() { 
    cls(); 
    world[visibleRoomIdx].draw(); 
    refreshLegend(); 
    drawPlayers(); 
}

/*      (__)
'\------(oo)    Convenience Wrappers 
  ||    (__)
  ||w--||                         */


void Game::placeBomb(int roomIdx, const Point& pos, int delay) {
    Bomb::place(bombs, roomIdx, pos, delay);
}

struct SpringData* Game::findSpringAt(int roomIdx, const Point& p) {
    return SpringData::findAt(world[roomIdx], p);
}

struct SwitchData* Game::findSwitchAt(int roomIdx, const Point& p) {
    return SwitchData::findAt(world[roomIdx], p);
}

SpecialDoor* Game::findSpecialDoorAt(int roomIdx, const Point& p) {
    return SpecialDoor::findAt(world[roomIdx], p);
}

Obstacle* Game::findObstacleAt(int roomIdx, const Point& p) {
    return Obstacle::findAt(world[roomIdx], roomIdx, p);
}

// Rescan obstacles across all rooms to keep obstacle instances in sync after moves
void Game::rescanObstacles() {
    Obstacle::scanAllObstacles(world, roomConnections);
}