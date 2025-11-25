#pragma once
#include <vector>
#include "Screen.h"
#include "Player.h"

// Holds data for a pending room transition request
struct RoomTransition {
    Player* player;
    int nextRoom;
};

class Game {
private:
    std::vector<Screen> world;
    std::vector<Player> players;
    int visibleRoomIdx;
    bool isRunning;

    // Helper methods
    void init();
    void handleInput();
    void update();
    void processTransitions(std::vector<RoomTransition>& transitions);
    void drawEverything();

public:
    // Constructor
    Game(); // The main loop
    void run();
};