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
constexpr int GAME_TICK_DELAY_MS = 90;
constexpr int FINAL_ROOM_INDEX = 6;
constexpr int FINAL_ROOM_FOCUS_TICKS = 25; // ~2.25 seconds focus on final room

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
    
    std::vector<bool> playerReachedFinalRoom; // Track which players reached final room
    int finalRoomFocusTicks = 0; // countdown for camera focus on final room

    void initGame();

    void handleInput();
    void update();

    void drawPlayers();
    void drawEverything();
    void refreshLegend();

    void checkAndProcessTransitions();

    void handlePause();
    
public:
    
    Game();

    void start();
    static void runApp();

    bool isGameLost() const { 
        return heartsCount <= 0; 
    }

    const std::vector<Player>& getPlayers() const { return players; }
    std::vector<Player>& getPlayersMutable() { return players; }

    Obstacle* findObstacleAt(int roomIdx, const Point& p);
    struct SpringData* findSpringAt(int roomIdx, const Point& p);
    struct SwitchData* findSwitchAt(int roomIdx, const Point& p);
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
    
    void addPoints(int amount) { 
        pointsCount += amount; 
    }
    
    // Public helpers for Riddle class
    void refreshLegendPublic() { refreshLegend(); }
    void drawEverythingPublic() { drawEverything(); }
    
    // Access to riddles map for Riddle class
    std::map<RiddleKey, Riddle*>& getRiddlesByPosition() { return riddlesByPosition; }
    
    // Check if player reached final room
    bool hasPlayerReachedFinalRoom(size_t playerIdx) const {
        if (playerIdx >= playerReachedFinalRoom.size()) return false;
        return playerReachedFinalRoom[playerIdx];
    }
    
    // Mark player as reached final room
    void setPlayerReachedFinalRoom(size_t playerIdx, bool reached) {
        if (playerIdx < playerReachedFinalRoom.size()) {
            playerReachedFinalRoom[playerIdx] = reached;
        }
    }
};