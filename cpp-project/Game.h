#pragma once
#include <vector>
#include <map>
#include <memory>

#include "Screen.h"
#include "Player.h"
#include "Riddle.h"
#include "Bomb.h"
#include "Legend.h"
#include "RoomConnections.h"
#include "GameRecorder.h"
#include "GameState.h"

constexpr int ESC_KEY = 27;
constexpr int GAME_TICK_DELAY_MS = 90;
constexpr int GAME_TICK_DELAY_LOAD_MS = 30;  // Faster for load mode
constexpr int FINAL_ROOM_INDEX = 7;
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
    std::vector<Point> previousPlayerPositions; // Track previous positions for darkness updates
    
    // Game mode and recording/playback
    GameMode gameMode = GameMode::Normal;
    std::unique_ptr<GameRecorder> recorder;
    int gameCycle = 0;  // Game tick counter for recording/playback
    std::vector<std::string> loadedScreenFiles;  // Screen files used in this session
    bool inPauseMenu = false; // Track if we are in pause menu during playback

    void initGame();
    void initGame(const GameStateData& savedState);  // Initialize from saved state

    void handleInput();
    void handleInputFromRecorder();  // Handle input from recorded file
    void update();

    void drawPlayers();
    void drawEverything();
    void refreshLegend();
    void updatePressureButtons();

    void checkAndProcessTransitions();

    void handlePause();
    void handleSaveState();  // Handle saving game state (ESC -> S)
    
    // Recording helpers (private)
    void recordScreenTransition(int playerIndex, int targetScreen);
    void recordRiddleEvent(int playerIndex, const std::string& question, const std::string& answer, bool correct);
    void recordGameEnd(bool isWin);
    
public:
    
    Game();
    Game(GameMode mode);
    Game(const GameStateData& savedState, GameMode mode = GameMode::Normal);  // Load from saved state

    void start();
    static void runApp(GameMode mode = GameMode::Normal);

    bool isGameLost() const { 
        return heartsCount <= 0; 
    }

    const std::vector<Player>& getPlayers() const { return players; }
    std::vector<Player>& getPlayersMutable() { return players; }

    Obstacle* findObstacleAt(int roomIdx, const Point& p);
    class SpringData* findSpringAt(int roomIdx, const Point& p);
    class SwitchData* findSwitchAt(int roomIdx, const Point& p);
    SpecialDoor* findSpecialDoorAt(int roomIdx, const Point& p);

    int getVisibleRoomIdx() const { return visibleRoomIdx; }
    int getWorldSize() const { return (int)world.size(); }
    Screen& getScreen(int roomIdx) { return world[roomIdx]; }

    int getTargetRoom(int fromRoom, Direction dir) const { return roomConnections.getTargetRoom(fromRoom, dir); }
    
    void placeBomb(int roomIdx, const Point& pos, int delay = 5);
    void removeBombAt(int roomIdx, const Point& pos);
    
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

    // Rescan obstacles across all rooms to keep obstacle instances in sync after moves
    void rescanObstacles();
    
    // Get current game state for saving
    GameStateData captureState() const;
    
    // Get/set game mode
    GameMode getGameMode() const { return gameMode; }
    
    // Get recorder for external event recording (riddles, etc.)
    GameRecorder* getRecorder() { return recorder.get(); }
    int getGameCycle() const { return gameCycle; }
    int getHeartsCount() const { return heartsCount; }
    int getPointsCount() const { return pointsCount; }
    
    // Record life lost event (public for Bomb class)
    void recordLifeLost(int playerIndex);
};