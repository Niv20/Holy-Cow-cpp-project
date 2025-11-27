#pragma once
#include <vector>
#include <map>
#include "Screen.h"
#include "Player.h"
#include "Riddle.h"
#include "Bomb.h"
#include "Legend.h"
#include "ScreenLoader.h"


constexpr int ESC_KEY = 27;

// Holds data for a pending room transition request
struct RoomTransition {
    Player* player;
    int nextRoom;
};

class Game {
private:
    std::vector<Screen> world;
    std::vector<Player> players;
    std::map<int, Riddle*> riddlesByRoom; // Map room index to riddle
    int visibleRoomIdx;
    bool isRunning;
    int pointsCount = 0;
    int heartsCount = 3;

    // Active bombs
    std::vector<Bomb> bombs;

    // Legend manager
    Legend legend;

    // Helper methods
    void init();
    void handleInput();
    void update();
    void processTransitions(std::vector<RoomTransition>& transitions);
    void drawEverything();
    void handleRiddleEncounter(Player& player);

    void syncBombsInRoom(int roomIdx);
    void tickAndHandleBombs();
    void explodeBomb(const Bomb& b);

public:
    // Constructor
    Game(); // The main loop
    void run();
};