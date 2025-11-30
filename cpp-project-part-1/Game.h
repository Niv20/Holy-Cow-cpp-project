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

// Key for riddle lookup: room index + position
struct RiddleKey {
    int roomIdx;
    int x;
    int y;
    
    bool operator<(const RiddleKey& other) const {
        if (roomIdx != other.roomIdx) return roomIdx < other.roomIdx;
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
};

class Game {
private:
    std::vector<Screen> world;
    std::vector<Player> players;
    std::map<RiddleKey, Riddle*> riddlesByPosition; // Map position to riddle
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
    void handlePause();

    void syncBombsInRoom(int roomIdx);
    void tickAndHandleBombs();
    void explodeBomb(const Bomb& b);

    void refreshLegend();
    void drawPlayers();

public:
    // Constructor
    Game();
    
    // The main loop
    void run();
    
    // Check if player lost (hearts <= 0)
    bool isGameLost() const { return heartsCount <= 0; }
};