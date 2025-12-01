#pragma once
#include <vector>
#include <map>
#include "Screen.h"
#include "Player.h"
#include "Riddle.h"
#include "Bomb.h"
#include "Legend.h"
#include "ScreenLoader.h"
#include "RoomConnections.h"
#include "Obstacle.h"
#include "Spring.h"
#include "Switch.h"

constexpr int ESC_KEY = 27;

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
    RoomConnections roomConnections;
    int visibleRoomIdx;
    bool isRunning;
    int pointsCount = 0;
    int heartsCount = 3;

    // Active bombs
    std::vector<Bomb> bombs;

    // Obstacles scanned across all rooms
    std::vector<Obstacle> obstacles;

    // Springs scanned across all rooms
    std::vector<SpringData> springs;
    
    // Switches scanned across all rooms
    std::vector<SwitchData> switches;

    // Legend manager
    Legend legend;

    // Helper methods
    void init();
    void scanObstacles();
    void scanSprings(); // new: scan all springs in all rooms
    void scanSwitches(); // new: scan all switches in all rooms
    void handleInput();
    void update();
    void checkAndProcessTransitions();
    void drawEverything();
    void handleRiddleEncounter(Player& player);
    void handlePause();

    void syncBombsInRoom(int roomIdx);
    void tickAndHandleBombs();
    void explodeBomb(const Bomb& b);

    void refreshLegend();
    void drawPlayers();
public:
    Game();
    void run();
    bool isGameLost() const { return heartsCount <= 0; }

    // Accessors used by Player for obstacle logic
    const std::vector<Player>& getPlayers() const { return players; }
    std::vector<Player>& getPlayersMutable() { return players; }
    Obstacle* findObstacleAt(int roomIdx, const Point& p);
    SpringData* findSpringAt(int roomIdx, const Point& p); // new
    SwitchData* findSwitchAt(int roomIdx, const Point& p); // new
    int getVisibleRoomIdx() const { return visibleRoomIdx; }
    Screen& getScreen(int roomIdx) { return world[roomIdx]; }
    const std::vector<Obstacle>& getObstacles() const { return obstacles; }

    // New: expose room connection for obstacle movement
    int getTargetRoom(int fromRoom, Direction dir) const { return roomConnections.getTargetRoom(fromRoom, dir); }
};