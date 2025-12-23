#include <conio.h>
#include <windows.h>
#include <queue>
#include <set>
#include <vector>
#include <string>
#include <utility>
#include <iostream>

#include "Game.h"
#include "utils.h"
#include "Glyph.h"
#include "Menu.h"
#include "RoomConnections.h"
#include "Player.h"
#include "Point.h"
#include "Riddle.h"
#include "Obstacle.h"
#include "FileParser.h"
#include "DarkRoom.h"

using std::vector;
using std::string;
using std::queue;
using std::pair;
using std::set;


/*      (__)
'\------(oo)    Constructor
  ||    (__)
  ||w--||                */

Game::Game() : visibleRoomIdx(0), isRunning(true) { 
    initGame(); 
}

void Game::initGame() {

    world = Screen::loadScreensFromFiles();
 
    if (world.empty()) { 
        FileParser::reportError("Cannot start game: No level screens could be loaded.");
        std::cerr << "Please ensure adv-world_*.screen files are present next to the executable." << std::endl;
        isRunning = false; 
        return; 
    }

    // Load room connections from screen metadata
    roomConnections.loadFromScreens(world);

    Screen::scanAllScreens(world, roomConnections, riddlesByPosition, legend);

    players.push_back(Player(Point(53, 19), "wdxase", Glyph::First_Player, 0));
    players.push_back(Player(Point(63, 19), "ilmjko", Glyph::Second_Player, 0));

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
try {
    SetConsoleOutputCP(65001); 
    setConsoleFont(); 
    hideCursor();
} catch (...) {
    FileParser::reportError("Warning: Could not initialize console settings");
}

bool exitProgram = false;

while (!exitProgram) {

    MenuAction action = Menu::showStartMenu();
        switch (action) {

            case MenuAction::NewGame: {
                Game game; 
                if (!game.isRunning) {
                    // Game failed to initialize, show error and return to menu
                    std::cerr << "Press any key to return to menu..." << std::endl;
                    _getch();
                } else {
                    game.start();
                }
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

// Get the correct message lines
std::string line1, line2, line3;
DarkRoomManager::getDarkRoomMessage(world[visibleRoomIdx], players, visibleRoomIdx, line1, line2, line3);

// Render message box content from metadata (if exists)
world[visibleRoomIdx].renderMessageBox(line1, line2, line3);

// Use darkness-aware drawing if room has dark zones
if (DarkRoomManager::roomHasDarkness(world[visibleRoomIdx])) {
    DarkRoomManager::drawWithDarkness(world[visibleRoomIdx], players, visibleRoomIdx);
} else {
    world[visibleRoomIdx].draw();
}
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
                // Use drawEverything to properly handle darkness rendering
                drawEverything();
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
    // Collect positions of players in current room
    std::vector<std::pair<Point, size_t>> playerPositions;
    for (size_t i = 0; i < players.size(); ++i) {
        if (players[i].getRoomIdx() == visibleRoomIdx) {
            playerPositions.push_back({players[i].getPosition(), i});
        }
    }
    
    // Draw players, handling overlapping positions
    std::set<std::pair<int,int>> drawnPositions;
    for (const auto& [pos, idx] : playerPositions) {
        auto posKey = std::make_pair(pos.getX(), pos.getY());
        if (drawnPositions.count(posKey) == 0) {
            // Check if another player is at the same position
            bool overlapping = false;
            for (const auto& [otherPos, otherIdx] : playerPositions) {
                if (otherIdx != idx && otherPos.getX() == pos.getX() && otherPos.getY() == pos.getY()) {
                    overlapping = true;
                    break;
                }
            }
            
            if (overlapping) {
                // Draw a combined symbol when players overlap
                HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
                COORD c{ (SHORT)pos.getX(), (SHORT)pos.getY() };
                SetConsoleCursorPosition(hOut, c);
                wchar_t combined = L'O'; // Simple ASCII char to show both players (works on all systems)
                DWORD written;
                WriteConsoleW(hOut, &combined, 1, &written, nullptr);
            } else {
                players[idx].draw();
            }
            drawnPositions.insert(posKey);
        }
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

// Track player state before movement (room, position, carried) to detect relevant changes
struct PlayerSnapshot {
    int roomIdx;
    Point pos;
    char carried;
};
std::vector<PlayerSnapshot> beforeSnapshots;
beforeSnapshots.reserve(players.size());
for (const auto& p : players) {
    beforeSnapshots.push_back({ p.getRoomIdx(), p.getPosition(), p.getCarried() });
}

// Track player rooms before movement to detect teleportation
std::vector<int> roomsBefore;
for (const auto& p : players) {
    roomsBefore.push_back(p.getRoomIdx());
}

// Store previous positions for darkness update optimization
previousPlayerPositions.clear();
for (const auto& p : players) {
    if (p.getRoomIdx() == visibleRoomIdx) {
        previousPlayerPositions.push_back(p.getPosition());
    }
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

// Update pressure buttons after all movement to reflect new player positions
updatePressureButtons();

// Update darkness when players in room move or when torch state changes (carry/pick/drop or torch carrier enters/leaves)
if (DarkRoomManager::roomHasDarkness(world[visibleRoomIdx])) {
    bool needsDarkUpdate = false;
    bool torchChange = false;

    for (size_t i = 0; i < players.size(); ++i) {
        const auto& before = beforeSnapshots[i];
        const auto& after = players[i];

        bool beforeInRoom = before.roomIdx == visibleRoomIdx;
        bool afterInRoom = after.getRoomIdx() == visibleRoomIdx;
        bool beforeTorch = before.carried == '!';
        bool afterTorch = after.getCarried() == '!';

        // Any movement of a player currently in the room requires redraw (players are drawn via darkness layer)
        if (beforeInRoom || afterInRoom) {
            if (before.pos.getX() != after.getPosition().getX() || before.pos.getY() != after.getPosition().getY() || beforeInRoom != afterInRoom) {
                needsDarkUpdate = true;
            }
        }

        // Torch carrier entered or left the visible room
        if (beforeInRoom != afterInRoom && (beforeTorch || afterTorch)) {
            torchChange = true;
        }

        // Torch picked up or dropped, or torch moved
        if ((beforeTorch != afterTorch) || ((beforeTorch || afterTorch) && (before.pos.getX() != after.getPosition().getX() || before.pos.getY() != after.getPosition().getY()))) {
            torchChange = true;
        }
    }

    if (needsDarkUpdate) {
        // Collect dropped torch positions so they contribute light/halo
        std::vector<Point> torchSources;
        if (torchChange) {
            for (int y = 0; y < Screen::MAX_Y; ++y) {
                for (int x = 0; x < Screen::MAX_X; ++x) {
                    Point p{x, y};
                    if (Glyph::isTorch(world[visibleRoomIdx].getCharAt(p))) {
                        torchSources.push_back(p);
                    }
                }
            }
        }

        DarkRoomManager::updateDarknessAroundPlayers(world[visibleRoomIdx], players,
                                                     visibleRoomIdx, previousPlayerPositions, torchSources);
    }
}
    
// Check for teleportation (room changed without edge transition)
for (size_t i = 0; i < players.size(); ++i) {
    int roomBefore = roomsBefore[i];
    int roomAfter = players[i].getRoomIdx();
    if (roomBefore != roomAfter && roomBefore == visibleRoomIdx) {
        // Player teleported to a different room - update camera
        bool anyPlayerInCurrentRoom = false;
        for (const auto& p : players) {
            if (p.getRoomIdx() == visibleRoomIdx) {
                anyPlayerInCurrentRoom = true;
                break;
            }
        }
        if (!anyPlayerInCurrentRoom) {
            visibleRoomIdx = roomAfter;
            drawEverything();
        }
    }
}
    
// Draw players only if room doesn't have darkness (darkness update includes players)
if (!DarkRoomManager::roomHasDarkness(world[visibleRoomIdx])) {
    drawPlayers();
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
        if (!DarkRoomManager::roomHasDarkness(world[visibleRoomIdx])) {
            drawPlayers();
        }
        return;
    }
    
    Bomb::tickAndHandleAll(bombs, *this);
    refreshLegend(); 
    if (!DarkRoomManager::roomHasDarkness(world[visibleRoomIdx])) {
        drawPlayers();
    }
}

void Game::updatePressureButtons() {
    for (size_t roomIdx = 0; roomIdx < world.size(); ++roomIdx) {
        Screen& screen = world[roomIdx];
        auto& buttons = screen.getDataMutable().pressureButtons;
        if (buttons.empty()) continue;

        struct TargetState {
            wchar_t original = Glyph::Empty;
            bool hasOriginal = false;
            int activeCount = 0;
        };

        std::map<std::pair<int, int>, TargetState> targets;

        std::vector<Point> roomPlayers;
        for (const auto& pl : players) {
            if (pl.getRoomIdx() == (int)roomIdx) {
                roomPlayers.push_back(pl.getPosition());
            }
        }

        for (auto& ps : buttons) {
            bool active = false;
            Point psPos = ps.getPos();
            for (const auto& pp : roomPlayers) {
                if (pp.getX() == psPos.getX() && pp.getY() == psPos.getY()) {
                    active = true;
                    break;
                }
            }

            for (const auto& tgt : ps.getTargets()) {
                Point tgtPos = tgt.getPos();
                auto key = std::make_pair(tgtPos.getX(), tgtPos.getY());
                auto& state = targets[key];
                if (!state.hasOriginal) {
                    state.original = tgt.getOriginalChar();
                    state.hasOriginal = true;
                }
                if (active) {
                    state.activeCount++;
                }
            }
        }

        bool isVisibleRoom = (visibleRoomIdx == (int)roomIdx);
        bool isDark = DarkRoomManager::roomHasDarkness(screen);

        for (auto& kv : targets) {
            Point p{ kv.first.first, kv.first.second };
            wchar_t desired = (kv.second.activeCount > 0) ? Glyph::Empty : kv.second.original;
            if (screen.getCharAt(p) != desired) {
                screen.setCharAt(p, desired);
                if (isVisibleRoom) {
                    if (isDark) {
                        DarkRoomManager::refreshCellWithDarkness(screen, p, players, (int)roomIdx);
                    } else {
                        screen.refreshCell(p);
                    }
                }
            }
        }
    }
}

// Written by AI! Thanks Gemini :)
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
        if (pos.getX() <= 0)
            dir = Direction::Left;
        else if (pos.getX() >= maxX - 1)
            dir = Direction::Right;
        else if (pos.getY() <= 0)
            dir = Direction::Up;
        else if (pos.getY() >= maxY - 1)
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
            case Direction::Left:  s = Point(maxX - 2, t.originalPos.getY()); break;
            case Direction::Right: s = Point(1,        t.originalPos.getY()); break;
            case Direction::Up:    s = Point(t.originalPos.getX(), maxY - 2); break;
            case Direction::Down:  s = Point(t.originalPos.getX(), 1);        break;
            default:               s = t.originalPos; break;
        }
        if (s.getX() < 1) s.setX(1); if (s.getX() > maxX - 2) s.setX(maxX - 2);
        if (s.getY() < 1) s.setY(1); if (s.getY() > maxY - 2) s.setY(maxY - 2);
        spawns[i] = s;
    }

    // Helper to check if a position is valid for spawning
    auto isValidSpawn = [&](const Point& p, int roomIdx, const vector<Point>& existingSpawns, size_t upToIdx) {
        // Check bounds
        if (p.getX() < 1 || p.getX() > maxX - 2 || p.getY() < 1 || p.getY() > maxY - 2)
            return false;
        // Check not a wall
        wchar_t ch = world[roomIdx].getCharAt(p);
        if (Glyph::isWall(ch))
            return false;
        // Check not colliding with earlier spawns in this transition batch
        for (size_t k = 0; k < upToIdx; ++k) {
            if (existingSpawns[k].getX() == p.getX() && existingSpawns[k].getY() == p.getY())
                return false;
        }
        // Check not colliding with any player already in target room
        for (const auto& pl : players) {
            if (pl.getRoomIdx() == roomIdx) {
                Point plPos = pl.getPosition();
                if (plPos.getX() == p.getX() && plPos.getY() == p.getY())
                    return false;
            }
        }
        return true;
    };

    // Ensure no two players spawn at the same position, and no spawn on existing player
    for (size_t idx = 0; idx < spawns.size(); ++idx) {
        int roomIdx = transitions[idx].targetRoom;
        Direction dir = transitions[idx].direction;
        
        // Check if current spawn is valid
        if (!isValidSpawn(spawns[idx], roomIdx, spawns, idx)) {
            // Need to find alternative position
            bool placed = false;
            
            // Try offsets in perpendicular direction first, then along entry direction
            for (int offset = 1; offset <= 10 && !placed; ++offset) {
                Point candidates[4];
                if (dir == Direction::Left || dir == Direction::Right) {
                    // Entry is horizontal, try vertical offsets first
                    candidates[0] = Point(spawns[idx].getX(), spawns[idx].getY() + offset);
                    candidates[1] = Point(spawns[idx].getX(), spawns[idx].getY() - offset);
                    candidates[2] = Point(spawns[idx].getX() + offset, spawns[idx].getY());
                    candidates[3] = Point(spawns[idx].getX() - offset, spawns[idx].getY());
                } else {
                    // Entry is vertical, try horizontal offsets first
                    candidates[0] = Point(spawns[idx].getX() + offset, spawns[idx].getY());
                    candidates[1] = Point(spawns[idx].getX() - offset, spawns[idx].getY());
                    candidates[2] = Point(spawns[idx].getX(), spawns[idx].getY() + offset);
                    candidates[3] = Point(spawns[idx].getX(), spawns[idx].getY() - offset);
                }
                
                for (int c = 0; c < 4 && !placed; ++c) {
                    if (isValidSpawn(candidates[c], roomIdx, spawns, idx)) {
                        spawns[idx] = candidates[c];
                        placed = true;
                    }
                }
            }
        }
    }

    // Apply transitions per player
    for (size_t i = 0; i < transitions.size(); ++i) {
        Player* pl = transitions[i].player;
        Point newPos = spawns[i];
        newPos.setDiffX(transitions[i].originalPos.getDiffX());
        newPos.setDiffY(transitions[i].originalPos.getDiffY());
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
    
    // Get the correct message lines
    std::string line1, line2, line3;
    DarkRoomManager::getDarkRoomMessage(world[visibleRoomIdx], players, visibleRoomIdx, line1, line2, line3);

    // Render message box content from metadata (if exists)
    world[visibleRoomIdx].renderMessageBox(line1, line2, line3);
    
    // Use darkness-aware drawing if room has dark zones
    if (DarkRoomManager::roomHasDarkness(world[visibleRoomIdx])) {
        DarkRoomManager::drawWithDarkness(world[visibleRoomIdx], players, visibleRoomIdx);
    } else {
        world[visibleRoomIdx].draw();
    }
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


void Game::removeBombAt(int roomIdx, const Point& pos) {
    // Remove bomb at specific position (when player picks it up)
    bombs.erase(
        std::remove_if(bombs.begin(), bombs.end(),
            [roomIdx, &pos](const Bomb& b) {
                return b.getRoomIdx() == roomIdx && 
                       b.getPosition().getX() == pos.getX() && 
                       b.getPosition().getY() == pos.getY();
            }),
        bombs.end()
    );
}

class SpringData* Game::findSpringAt(int roomIdx, const Point& p) {
    return SpringData::findAt(world[roomIdx], p);
}

class SwitchData* Game::findSwitchAt(int roomIdx, const Point& p) {
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