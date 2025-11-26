#include "Game.h"
#include "MapData.h" // Access to the raw map data
#include "utils.h"   // Access to gotoxy, hideCursor, cls
#include <conio.h>   // _kbhit, _getch
#include <windows.h> // Sleep
#include <cctype>

using namespace std;

// constructor
Game::Game() : visibleRoomIdx(0), isRunning(true) {
    init();
}

void Game::init() {
    // 1. Load Maps
    world.push_back(Screen(room0_raw)); // Room 0
    world.push_back(Screen(room1_raw)); // Room 1
    world.push_back(Screen(room2_raw)); // Room 2

    // 2. Create Players
    // Player 1: @, keys: wdxas, Room 0
    players.push_back(Player(Point(5, 2), "wdxas", '@', 0));

    // Player 2: #, keys: ilmjk, Room 0
    players.push_back(Player(Point(5, 3), "ilmjk", '#', 0));
}

void Game::run() {

	// Hide the cursor for better visuals
    hideCursor();

    // Draw initial state
    drawEverything();

    while (isRunning) {
		handleInput();
        update();
        Sleep(100);
    }

    cls();
}

void Game::drawEverything() {
    cls(); // Clear screen before redraw to prevent artifacts
    // Draw the room walls
    world[visibleRoomIdx].draw();
    
    // Draw all players present in this room
    for (const auto& p : players) {
        if (p.getRoomIdx() == visibleRoomIdx) {
            p.draw();
        }
    }
}

void Game::handleInput() {

    if (_kbhit()) {
        char key = _getch();

		if (key == 27) { // ESC TODO: constexpr!!!
            isRunning = false;
			// Show pause message !!!
            return;
        }

        for (auto& p : players) {
            // Only allow control if the player is visible
            if (p.getRoomIdx() == visibleRoomIdx) {
                p.handleKey(key);
            }
        }
    }
}

void Game::update() {
    std::vector<RoomTransition> transitions;

    // --- 1. Collect Moves & Transition Requests ---
    for (auto& p : players) {
        if (p.getRoomIdx() == visibleRoomIdx) {
            p.move(world[visibleRoomIdx]);

            Point pos = p.getPosition();
            char cell = world[visibleRoomIdx].getCharAt(pos);
            
            if (isdigit(cell)) { 
                transitions.push_back({&p, cell - '0'});
            }
        }
    }

    // --- 2. Process all transitions at once ---
    if (!transitions.empty()) {
        processTransitions(transitions);
    }
}

void Game::processTransitions(std::vector<RoomTransition>& transitions) {
    int lastTransitionerNextRoom = -1;

    // Erase players leaving current visible room (without altering map data like door numbers)
    for (auto& trans : transitions) {
        Player* p = trans.player;
        int originRoom = p->getRoomIdx();

        // Only erase if currently on the visible screen (always true by collection logic, but safe check)
        if (originRoom == visibleRoomIdx) {
            Screen& originScreen = world[originRoom];
            // Redraw underlying tile (hide digits visually)
            char under = originScreen.getCharAt(p->getPosition());
            if (isdigit(under)) under = ' ';
            p->getPosition().draw(under);
        }

        // Move player logically to next room
        int nextRoom = trans.nextRoom;
        lastTransitionerNextRoom = nextRoom;
        p->setRoomIdx(nextRoom);

        // Compute spawn position based on side exited
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

        // Handle collision with occupant in destination room (simple push)
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

    // Decide if camera should move: move only if no players remain in current visible room
    bool cameraShouldMove = true;
    for (const auto& p : players) {
        if (p.getRoomIdx() == visibleRoomIdx) {
            cameraShouldMove = false; // Someone stayed -> keep camera
            break;
        }
    }

    if (cameraShouldMove && lastTransitionerNextRoom != -1) {
        visibleRoomIdx = lastTransitionerNextRoom;
        drawEverything(); // Draw new room with its players
    }
    // Else: do nothing (camera stays, transitioned player is now hidden in its new room)
}