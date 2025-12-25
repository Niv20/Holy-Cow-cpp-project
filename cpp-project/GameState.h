#pragma once
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include "Point.h"

// Class to hold a player's state for serialization
class PlayerState {
public:
    PlayerState();
    PlayerState(int room, int x, int y, int diffX, int diffY, char carried);

    int getRoomIdx() const;
    int getX() const;
    int getY() const;
    int getDiffX() const;
    int getDiffY() const;
    char getCarried() const;

    void setRoomIdx(int v);
    void setX(int v);
    void setY(int v);
    void setDiffX(int v);
    void setDiffY(int v);
    void setCarried(char v);

private:
    int roomIdx_;
    int x_;
    int y_;
    int diffX_;
    int diffY_;
    char carried_;
};

// Class to hold complete game state for save/load
class GameStateData {
public:
    GameStateData();

    // Metadata
    void setSaveName(const std::string& name);
    const std::string& getSaveName() const;
    void setTimestamp(std::time_t t);
    std::time_t getTimestamp() const;

    // Basic game state
    void setVisibleRoomIdx(int idx);
    int getVisibleRoomIdx() const;
    void setHeartsCount(int h);
    int getHeartsCount() const;
    void setPointsCount(int p);
    int getPointsCount() const;
    void setGameCycle(int c);
    int getGameCycle() const;

    // Players
    void addPlayerState(const PlayerState& ps);
    const std::vector<PlayerState>& getPlayers() const;
    std::vector<PlayerState>& getPlayersMutable();
    void setPlayers(const std::vector<PlayerState>& players);

    // Final room flags
    void setPlayerReachedFinalRoom(const std::vector<bool>& flags);
    const std::vector<bool>& getPlayerReachedFinalRoom() const;
    std::vector<bool>& getPlayerReachedFinalRoomMutable();

    // Screen files used (for verification)
    void setScreenFiles(const std::vector<std::string>& files);
    const std::vector<std::string>& getScreenFiles() const;

    // Screen modifications
    std::map<int, std::vector<std::tuple<int, int, wchar_t>>>& getScreenModificationsMutable();
    const std::map<int, std::vector<std::tuple<int, int, wchar_t>>>& getScreenModifications() const;

    // Riddle states
    void setRiddleStates(const std::vector<std::tuple<int, int, int, bool>>& states);
    const std::vector<std::tuple<int, int, int, bool>>& getRiddleStates() const;
    std::vector<std::tuple<int, int, int, bool>>& getRiddleStatesMutable();

private:
    std::string saveName_;
    std::time_t timestamp_;

    int visibleRoomIdx_;
    int heartsCount_;
    int pointsCount_;
    int gameCycle_;

    std::vector<PlayerState> players_;
    std::vector<bool> playerReachedFinalRoom_;

    std::vector<std::string> screenFiles_;

    std::map<int, std::vector<std::tuple<int, int, wchar_t>>> screenModifications_;

    std::vector<std::tuple<int, int, int, bool>> riddleStates_;
};

// Class for saving and loading game state
class GameState {
public:
    // Save directory name
    static constexpr const char* SAVES_DIR = "saves";
    static constexpr const char* SAVE_EXTENSION = ".sav";
    
    GameState();
    
    // Save current game state with a user-provided name
    bool saveState(const GameStateData& state, const std::string& saveName);
    
    // Load game state from a save file
    bool loadState(const std::string& saveFilePath, GameStateData& state);
    
    // Get list of available save files
    static std::vector<std::pair<std::string, std::string>> getAvailableSaves(); // returns (filename, display name)
    
    // Delete a save file
    static bool deleteSave(const std::string& saveFilePath);
    
    // Format timestamp for display
    static std::string formatTimestamp(std::time_t timestamp);
    
    // Generate a default save name based on current time
    static std::string generateDefaultSaveName();
    
private:
    // Ensure saves directory exists
    static bool ensureSavesDirectory();
    
    // Build full path for a save file
    static std::string buildSavePath(const std::string& saveName);
    
    // Serialize/deserialize helpers
    static std::string serializeState(const GameStateData& state);
    static bool deserializeState(const std::string& content, GameStateData& state);
    
    // Escape special characters in strings
    static std::string escapeString(const std::string& str);
    static std::string unescapeString(const std::string& str);
    
    // Helper for loading state (used by getAvailableSaves)
    static bool loadStaticHelper(const std::string& filepath, GameStateData& state);
};
