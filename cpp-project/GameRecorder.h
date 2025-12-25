#pragma once
#include <string>
#include <vector>
#include <fstream>
#include "Point.h"

// Enum for game run modes
enum class GameMode {
    Normal,     // Regular play without recording
    Save,       // Record game to files
    Load,       // Play from files
    LoadSilent  // Play from files silently (testing)
};

// Event types that get recorded
enum class GameEventType {
    KeyPress,           // Player pressed a key
    ScreenTransition,   // Player moved to another screen
    LifeLost,           // Player lost a life
    RiddleEncounter,    // Player encountered a riddle
    RiddleAnswer,       // Player answered a riddle
    GameEnd             // Game ended (win/lose)
};

// A single recorded game event
class GameEvent {
public:
    GameEvent();

    // Getters
    int getCycle() const;
    GameEventType getType() const;
    int getPlayerIndex() const;
    char getKeyPressed() const;
    int getTargetScreen() const;
    const std::string& getRiddleQuestion() const;
    const std::string& getRiddleAnswer() const;
    bool isRiddleCorrect() const;
    int getScore() const;
    bool getIsWin() const;

    // Setters
    void setCycle(int c);
    void setType(GameEventType t);
    void setPlayerIndex(int idx);
    void setKeyPressed(char k);
    void setTargetScreen(int t);
    void setRiddleQuestion(const std::string& q);
    void setRiddleAnswer(const std::string& a);
    void setRiddleCorrect(bool v);
    void setScore(int s);
    void setIsWin(bool v);

private:
    int cycle_;
    GameEventType type_;
    int playerIndex_;
    char keyPressed_;
    int targetScreen_;
    std::string riddleQuestion_;
    std::string riddleAnswer_;
    bool riddleCorrect_;
    int score_;
    bool isWin_;
};

// Result entry for the .result file
class ResultEntry {
public:
    ResultEntry();
    ResultEntry(int c, const std::string& desc);

    int getCycle() const;
    const std::string& getDescription() const;

    void setCycle(int c);
    void setDescription(const std::string& d);

private:
    int cycle_;
    std::string description_;
};

// Class for recording and playing back game sessions
class GameRecorder {
public:
    // File names
    static constexpr const char* STEPS_FILE = "adv-world.steps";
    static constexpr const char* RESULT_FILE = "adv-world.result";
    
    GameRecorder();
    ~GameRecorder();
    
    // Initialize for save or load mode
    bool initForSave(const std::vector<std::string>& screenFiles);
    bool initForLoad();
    
    // Set random seed (for reproducibility)
    void setRandomSeed(unsigned int seed);
    unsigned int getRandomSeed() const { return randomSeed_; }
    
    // Recording methods (save mode)
    void recordKeyPress(int cycle, int playerIndex, char key);
    void recordScreenTransition(int cycle, int playerIndex, int targetScreen);
    void recordLifeLost(int cycle, int playerIndex);
    void recordRiddleEncounter(int cycle, int playerIndex, const std::string& question);
    void recordRiddleAnswer(int cycle, int playerIndex, const std::string& answer, bool correct);
    void recordGameEnd(int cycle, int score, bool isWin);
    
    // Finalize and write files (save mode)
    bool finalizeRecording();
    
    // Playback methods (load mode)
    bool hasNextEvent() const;
    const GameEvent& peekNextEvent() const;
    GameEvent consumeNextEvent();
    bool shouldProcessEvent(int currentCycle) const;
    
    // Get expected results for verification (load mode)
    const std::vector<ResultEntry>& getExpectedResults() const { return expectedResults_; }
    
    // Add actual result for comparison (load mode)
    void addActualResult(int cycle, const std::string& description);
    
    // Compare actual vs expected results (load silent mode)
    bool verifyResults(std::string& errorMessage) const;
    
    // Get screen files that were recorded with
    const std::vector<std::string>& getScreenFiles() const { return screenFiles_; }
    
private:
    bool saveMode_;
    unsigned int randomSeed_;
    std::vector<std::string> screenFiles_;
    
    // For recording
    std::vector<GameEvent> recordedEvents_;
    std::vector<ResultEntry> recordedResults_;
    
    // For playback
    std::vector<GameEvent> loadedEvents_;
    size_t currentEventIndex_;
    std::vector<ResultEntry> expectedResults_;
    std::vector<ResultEntry> actualResults_;
    
    // File I/O helpers
    bool writeStepsFile();
    bool writeResultFile();
    bool readStepsFile();
    bool readResultFile();
    
    // Parsing helpers
    static std::string eventTypeToString(GameEventType type);
    static GameEventType stringToEventType(const std::string& str);
    static std::string escapeString(const std::string& str);
    static std::string unescapeString(const std::string& str);
};
