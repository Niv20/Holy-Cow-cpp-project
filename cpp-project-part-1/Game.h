#pragma once
#include <vector>
#include <map>

#include "Screen.h"
#include "Player.h"
#include "Riddle.h"
#include "Bomb.h"
#include "Legend.h"
#include "RoomConnections.h"

constexpr int ESC_KEY = 27;

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

    std::vector<Screen> world;
    std::vector<Player> players;
    RoomConnections roomConnections;

    int visibleRoomIdx;
    bool isRunning;
    int pointsCount = 0;
    int heartsCount = 3;

    std::vector<Bomb> bombs;
    std::map<RiddleKey, Riddle*> riddlesByPosition;

    Legend legend;

    void initGame();

    void handleInput();
    void update();

    void drawPlayers();
    void drawEverything();
    void refreshLegend();

    void checkAndProcessTransitions();

    void handlePause();
    void handleRiddleEncounter(Player& player);

    // Scan/load helpers
    void scanObstacles();
    void scanLegend();
    void scanRiddles();

    void syncBombsInRoom(int roomIdx);
    void tickAndHandleBombs();
    
public:

    Game();

    void run();

    bool isGameLost() const { 
        return heartsCount <= 0; 
    }

    static void runApp();

    const std::vector<Player>& getPlayers() const { return players; }
    std::vector<Player>& getPlayersMutable() { return players; }

    Obstacle* findObstacleAt(int roomIdx, const Point& p);
    SpringData* findSpringAt(int roomIdx, const Point& p);
    SwitchData* findSwitchAt(int roomIdx, const Point& p);
    SpecialDoor* findSpecialDoorAt(int roomIdx, const Point& p);

    int getVisibleRoomIdx() const { return visibleRoomIdx; }
    int getWorldSize() const { return (int)world.size(); }
    Screen& getScreen(int roomIdx) { return world[roomIdx]; }

    int getTargetRoom(int fromRoom, Direction dir) const { return roomConnections.getTargetRoom(fromRoom, dir); }
    
    void placeBomb(int roomIdx, const Point& pos, int delay = 5);
    
    void reduceHearts(int amount) { 
        heartsCount -= amount; 
        if (heartsCount < 0) heartsCount = 0;
        if (heartsCount <= 0) isRunning = false;
    }
};