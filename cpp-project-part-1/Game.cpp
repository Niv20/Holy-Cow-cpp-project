#include "Game.h"
#include "MapData.h"
#include "RiddleData.h"
#include "utils.h"
#include "Tiles.h"
#include <conio.h>
#include <windows.h>

using namespace std;

// constructor
Game::Game() : visibleRoomIdx(0), isRunning(true) {
    init();
}

void Game::init() {
    // 1. Load Maps
    world.push_back(Screen(room0_raw));
    world.push_back(Screen(room1_raw));
    world.push_back(Screen(room2_raw));

    // 2. Create Players
    players.push_back(Player(Point(5, 2), "wdxase", Tiles::First_Player, 0));
    players.push_back(Player(Point(5, 3), "ilmjko", Tiles::Second_Player, 0));
    
    // 3. Load Riddles
    vector<RiddleData> riddles = initRiddles();
    for (auto& rd : riddles) {
        riddlesByRoom[rd.roomIdx] = new Riddle(rd.riddle);
    }

    // 4. Scan existing bombs drawn on maps into bombs vector
    for (int room = 0; room < (int)world.size(); ++room) {
        Screen& s = world[room];
        for (int y = 0; y < Screen::MAX_Y; ++y) {
            for (int x = 0; x < Screen::MAX_X; ++x) {
                Point p{ x, y };
                if (s.getCharAt(p) == Tiles::Bomb) {
                    bombs.emplace_back(p, room, 5);
                }
            }
        }
    }

    // 5. Legend init
    legend.ensureRooms(world.size());
    for (int room = 0; room < (int)world.size(); ++room) {
        legend.locateLegendForRoom(room, world[room]);
    }
}

void Game::run() {
    hideCursor();
    drawEverything();

    while (isRunning) {
        handleInput();
        update();
        Sleep(100);
    }

    // Cleanup
    for (auto& pair : riddlesByRoom) {
        delete pair.second;
    }
    cls();
}

void Game::drawEverything() {
    cls();
    world[visibleRoomIdx].draw();

    // Draw legend for the current room
    char p1Inv = players.size() > 0 ? players[0].getCarried() : ' ';
    char p2Inv = players.size() > 1 ? players[1].getCarried() : ' ';
    legend.drawLegend(visibleRoomIdx, world[visibleRoomIdx], heartsCount, pointsCount, p1Inv, p2Inv);
    
    for (const auto& p : players) {
        if (p.getRoomIdx() == visibleRoomIdx) {
            p.draw();
        }
    }
}

void Game::handleInput() {
    if (_kbhit()) {
        char key = _getch();

        if (key == ESC_KEY) {
            isRunning = false;
            return;
        }

        for (auto& p : players) {
            if (p.getRoomIdx() == visibleRoomIdx) {
                p.handleKey(key);
            }
        }
    }
}

void Game::update() {
    std::vector<RoomTransition> transitions;

    // --- 1. Collect Moves & Check for Riddles ---
    for (auto& p : players) {
        if (p.getRoomIdx() == visibleRoomIdx) {
            p.move(world[visibleRoomIdx]);

            Point pos = p.getPosition();
            char cell = world[visibleRoomIdx].getCharAt(pos);
            
            // Check for riddle encounter
            if (Tiles::isRiddle(cell)) {
                handleRiddleEncounter(p);
            }
            // Check for room transition
            else if (Tiles::isRoomTransition(cell)) { 
                transitions.push_back({&p, cell - '0'});
            }
        }
    }

    // --- 2. Handle bombs ---
    tickAndHandleBombs();

    // --- 3. Process all transitions at once ---
    if (!transitions.empty()) {
        processTransitions(transitions);
    }

    // Avoid full-screen redraw each frame to reduce flicker.
    // Only refresh legend overlay without clearing screen.
    char p1Inv = players.size() > 0 ? players[0].getCarried() : ' ';
    char p2Inv = players.size() > 1 ? players[1].getCarried() : ' ';
    legend.drawLegend(visibleRoomIdx, world[visibleRoomIdx], heartsCount, pointsCount, p1Inv, p2Inv);
}

void Game::handleRiddleEncounter(Player& player) {

    int roomIdx = player.getRoomIdx();
    
    // Check if there's a riddle in this room
    if (riddlesByRoom.find(roomIdx) == riddlesByRoom.end()) {
        return; // No riddle in this room
    }
    
    Riddle* riddle = riddlesByRoom[roomIdx];
    
    // Build and display the riddle screen
    vector<string> riddleScreenData = riddle->buildRiddleScreen(riddleScreen_raw);
    Screen riddleScreen(riddleScreenData);
    
    cls();
    riddleScreen.draw();
    
    // Wait for player answer
    char answer = '\0';
    
    while (true) {
        if (_kbhit()) {
            answer = _getch();
            
            if (answer == ESC_KEY) {
                // Cancel riddle - player stays in place, riddle remains
                // Move player back to previous position
                Point pos = player.getPosition();
                Point prevPos = pos;
                if (pos.diff_x != 0) prevPos.x -= pos.diff_x;
                if (pos.diff_y != 0) prevPos.y -= pos.diff_y;
                player.setPosition(prevPos);
                player.stop(); // STAY
                break; // Exit loop
            }
            else if (answer >= '1' && answer <= '4') {
                // Check answer
                if (answer == riddle->getCorrectAnswer()) {
                    // CORRECT ANSWER
                    pointsCount += riddle->getPoints(); // Award current point value
                    // Remove the '?' from the map - player can pass through
                    world[roomIdx].setCharAt(player.getPosition(), Tiles::Empty);
                    // Note: Keep previous movement direction (do NOT stop)
                } 
                else {
                    // WRONG ANSWER
                    riddle->halvePoints(); // Divide riddle points by 2
                    heartsCount--; // Lose one heart
                    
                    // Move player back to previous position and stop movement
                    Point pos = player.getPosition();
                    Point prevPos = pos;
                    if (pos.diff_x != 0) prevPos.x -= pos.diff_x;
                    if (pos.diff_y != 0) prevPos.y -= pos.diff_y;
                    player.setPosition(prevPos);
                    player.stop(); // STAY on wrong answer
                }
                break; // Exit loop after answering
            }
        }
    }
    
    // Redraw the game screen and return to loop; next update will read fresh cell
    drawEverything();
}

// Creat by AI
void Game::processTransitions(std::vector<RoomTransition>& transitions) {
    int lastTransitionerNextRoom = -1;

    for (auto& trans : transitions) {
        Player* p = trans.player;
        int originRoom = p->getRoomIdx();

        if (originRoom == visibleRoomIdx) {
            Screen& originScreen = world[originRoom];
            char under = originScreen.getCharAt(p->getPosition());
            if (Tiles::isRoomTransition(under)) under = Tiles::Empty;
            p->getPosition().draw(under);
        }

        int nextRoom = trans.nextRoom;
        lastTransitionerNextRoom = nextRoom;
        p->setRoomIdx(nextRoom);

        Point pos = p->getPosition();
        Point spawn = pos;
        const int maxX = Screen::MAX_X;
        const int maxY = Screen::MAX_Y;

        bool fromLeft   = (pos.x <= 1);
        bool fromRight  = (pos.x >= maxX - 2);
        bool fromTop    = (pos.y <= 1);
        bool fromBottom = (pos.y >= maxY - 2);

        if (fromLeft)       spawn.x = maxX - 2;
        else if (fromRight) spawn.x = 1;
        else if (fromTop)   spawn.y = maxY - 2;
        else if (fromBottom)spawn.y = 1;
        else { 
            if (spawn.x < maxX/2) spawn.x += 1; else spawn.x -= 1;
        }

        Player* occupant = nullptr;
        for (auto& other : players) {
            if (&other == p || other.getRoomIdx() != nextRoom) continue;
            Point op = other.getPosition();
            if (op.x == spawn.x && op.y == spawn.y) {
                occupant = &other;
                break;
            }
        }

        if (occupant != nullptr) {
            int dx = 0, dy = 0;
            if (fromRight)      dx = +1;
            else if (fromLeft)  dx = -1;
            else if (fromBottom)dy = +1;
            else if (fromTop)   dy = -1;
            else {
                Point dirProbe = p->getPosition();
                if (dirProbe.diff_x != 0) dx = dirProbe.diff_x;
                else if (dirProbe.diff_y != 0) dy = dirProbe.diff_y;
                else dx = 1;
            }
            Point occPos = occupant->getPosition();
            occPos.x += dx;
            occPos.y += dy;
            occupant->setPosition(occPos);
        }
        p->setPosition(spawn);
    }

    bool cameraShouldMove = true;
    for (const auto& p : players) {
        if (p.getRoomIdx() == visibleRoomIdx) {
            cameraShouldMove = false;
            break;
        }
    }

    if (cameraShouldMove && lastTransitionerNextRoom != -1) {
        visibleRoomIdx = lastTransitionerNextRoom;
        drawEverything();
    }
}

void Game::tickAndHandleBombs() {
    // Tick bombs and collect those that need to explode now
    vector<Bomb> nextBombs;
    vector<Bomb> toExplode;
    nextBombs.reserve(bombs.size());

    for (auto& b : bombs) {
        if (b.tick()) {
            toExplode.push_back(b);
        } else {
            nextBombs.push_back(b);
        }
    }

    // Replace with bombs that didn't explode
    bombs.swap(nextBombs);

    // Explode after ticking (so we don't process a bomb twice)
    for (const auto& b : toExplode) {
        explodeBomb(b);
    }
}

void Game::explodeBomb(const Bomb& b) {
    int room = b.getRoomIdx();
    Screen& s = world[room];
    Point center = b.getPosition();

    const int radius = 3;
    int minX = max(0, center.x - radius);
    int maxX = min(Screen::MAX_X - 1, center.x + radius);
    int minY = max(0, center.y - radius);
    int maxY = min(Screen::MAX_Y - 1, center.y + radius);

    // Affect tiles in the square range
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            Point p{ x, y };
            char c = s.getCharAt(p);

            // Remove special walls only (-, |)
            if (c == Tiles::Bombable_Wall_H || c == Tiles::Bombable_Wall_V) {
                s.setCharAt(p, Tiles::Empty);
            }

            // TODO: In the future, remove entire obstacle when any '*' inside range is hit
            // if (c == Tiles::Obstacle) { ... }
        }
    }

    // Affect players inside radius (reduce hearts)
    int hits = 0;
    for (auto& pl : players) {
        if (pl.getRoomIdx() != room) continue;
        Point pp = pl.getPosition();
        if (pp.x >= minX && pp.x <= maxX && pp.y >= minY && pp.y <= maxY) {
            ++hits;
        }
    }
    heartsCount -= hits; // if both hit, 2 hearts down
    if (heartsCount < 0) heartsCount = 0;
}