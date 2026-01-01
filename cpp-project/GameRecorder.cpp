#include "GameRecorder.h"
#include "FileParser.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <iostream>

// GameEvent implementation
GameEvent::GameEvent()
    : cycle_(0), type_(GameEventType::KeyPress), playerIndex_(0), keyPressed_('\0'),
      targetScreen_(-1), riddleCorrect_(false), score_(0), isWin_(false) {}

int GameEvent::getCycle() const { return cycle_; }
GameEventType GameEvent::getType() const { return type_; }
int GameEvent::getPlayerIndex() const { return playerIndex_; }
char GameEvent::getKeyPressed() const { return keyPressed_; }
int GameEvent::getTargetScreen() const { return targetScreen_; }
const std::string& GameEvent::getRiddleQuestion() const { return riddleQuestion_; }
const std::string& GameEvent::getRiddleAnswer() const { return riddleAnswer_; }
bool GameEvent::isRiddleCorrect() const { return riddleCorrect_; }
int GameEvent::getScore() const { return score_; }
bool GameEvent::getIsWin() const { return isWin_; }

void GameEvent::setCycle(int c) { cycle_ = c; }
void GameEvent::setType(GameEventType t) { type_ = t; }
void GameEvent::setPlayerIndex(int idx) { playerIndex_ = idx; }
void GameEvent::setKeyPressed(char k) { keyPressed_ = k; }
void GameEvent::setTargetScreen(int t) { targetScreen_ = t; }
void GameEvent::setRiddleQuestion(const std::string& q) { riddleQuestion_ = q; }
void GameEvent::setRiddleAnswer(const std::string& a) { riddleAnswer_ = a; }
void GameEvent::setRiddleCorrect(bool v) { riddleCorrect_ = v; }
void GameEvent::setScore(int s) { score_ = s; }
void GameEvent::setIsWin(bool v) { isWin_ = v; }

// ResultEntry implementation
ResultEntry::ResultEntry() : cycle_(0) {}
ResultEntry::ResultEntry(int c, const std::string& desc) : cycle_(c), description_(desc) {}

int ResultEntry::getCycle() const { return cycle_; }
const std::string& ResultEntry::getDescription() const { return description_; }
void ResultEntry::setCycle(int c) { cycle_ = c; }
void ResultEntry::setDescription(const std::string& d) { description_ = d; }

// Rest of GameRecorder implementation updated to use getters/setters

GameRecorder::GameRecorder() 
    : saveMode_(false), currentEventIndex_(0) {
}

GameRecorder::~GameRecorder() {
}

bool GameRecorder::initForSave(const std::vector<std::string>& screenFiles) {
    saveMode_ = true;
    screenFiles_ = screenFiles;
    recordedEvents_.clear();
    recordedResults_.clear();
    
    return true;
}

bool GameRecorder::initForLoad() {
    saveMode_ = false;
    currentEventIndex_ = 0;
    loadedEvents_.clear();
    expectedResults_.clear();
    actualResults_.clear();
    
    if (!readStepsFile()) {
        return false;
    }
    
    if (!readResultFile()) {
        // Result file is optional for load mode (not required for playback)
        // But we still need it for verification in silent mode
    }
    
    return true;
}

// Recording methods
void GameRecorder::recordKeyPress(int cycle, int playerIndex, char key) {
    if (!saveMode_) return;
    
    GameEvent event;
    event.setCycle(cycle);
    event.setType(GameEventType::KeyPress);
    event.setPlayerIndex(playerIndex);
    event.setKeyPressed(key);
    recordedEvents_.push_back(event);
}

void GameRecorder::recordScreenTransition(int cycle, int playerIndex, int targetScreen) {
    if (!saveMode_) return;
    
    GameEvent event;
    event.setCycle(cycle);
    event.setType(GameEventType::ScreenTransition);
    event.setPlayerIndex(playerIndex);
    event.setTargetScreen(targetScreen);
    recordedEvents_.push_back(event);
    
    // Add to results - screen transitions should be recorded
    std::ostringstream oss;
    oss << "Player " << (playerIndex + 1) << " moved to screen " << targetScreen;
    recordedResults_.emplace_back(cycle, oss.str());
}

void GameRecorder::recordLifeLost(int cycle, int playerIndex) {
    if (!saveMode_) return;
    
    GameEvent event;
    event.setCycle(cycle);
    event.setType(GameEventType::LifeLost);
    event.setPlayerIndex(playerIndex);
    recordedEvents_.push_back(event);
    
    // Add to results
    std::ostringstream oss;
    oss << "Player " << (playerIndex + 1) << " lost a life";
    recordedResults_.emplace_back(cycle, oss.str());
}

void GameRecorder::recordRiddleEncounter(int cycle, int playerIndex, const std::string& question) {
    if (!saveMode_) return;
    
    GameEvent event;
    event.setCycle(cycle);
    event.setType(GameEventType::RiddleEncounter);
    event.setPlayerIndex(playerIndex);
    event.setRiddleQuestion(question);
    recordedEvents_.push_back(event);
}

void GameRecorder::recordRiddleAnswer(int cycle, int playerIndex, const std::string& answer, bool correct) {
    if (!saveMode_) return;
    
    GameEvent event;
    event.setCycle(cycle);
    event.setType(GameEventType::RiddleAnswer);
    event.setPlayerIndex(playerIndex);
    event.setRiddleAnswer(answer);
    event.setRiddleCorrect(correct);
    recordedEvents_.push_back(event);
    
    // Add to results
    std::ostringstream oss;
    oss << "Player " << (playerIndex + 1) << " answered riddle: " << answer 
        << " (" << (correct ? "CORRECT" : "WRONG") << ")";
    recordedResults_.emplace_back(cycle, oss.str());
}

void GameRecorder::recordGameEnd(int cycle, int score, bool isWin) {
    if (!saveMode_) return;
    
    GameEvent event;
    event.setCycle(cycle);
    event.setType(GameEventType::GameEnd);
    event.setScore(score);
    event.setIsWin(isWin);
    recordedEvents_.push_back(event);
    
    // Add to results
    std::ostringstream oss;
    oss << "Game ended: " << (isWin ? "WIN" : "LOSE") << " with score " << score;
    recordedResults_.emplace_back(cycle, oss.str());
}

bool GameRecorder::finalizeRecording() {
    if (!saveMode_) return false;
    
    bool stepsOk = writeStepsFile();
    bool resultOk = writeResultFile();
    
    return stepsOk && resultOk;
}

// Playback methods
bool GameRecorder::hasNextEvent() const {
    return currentEventIndex_ < loadedEvents_.size();
}

const GameEvent& GameRecorder::peekNextEvent() const {
    static GameEvent empty;
    if (currentEventIndex_ >= loadedEvents_.size()) {
        return empty;
    }
    return loadedEvents_[currentEventIndex_];
}

GameEvent GameRecorder::consumeNextEvent() {
    if (currentEventIndex_ >= loadedEvents_.size()) {
        return GameEvent();
    }
    return loadedEvents_[currentEventIndex_++];
}

bool GameRecorder::shouldProcessEvent(int currentCycle) const {
    if (!hasNextEvent()) return false;
    return loadedEvents_[currentEventIndex_].getCycle() <= currentCycle;
}

void GameRecorder::addActualResult(int cycle, const std::string& description) {
    actualResults_.emplace_back(cycle, description);
}

bool GameRecorder::verifyResults(std::string& errorMessage) const {
    std::ostringstream oss;
    bool hasError = false;
    
    if (expectedResults_.empty() && actualResults_.empty()) {
        // Both empty is OK - no events to verify
        return true;
    }
    
    if (expectedResults_.empty()) {
        oss << "VERIFICATION FAILED: No expected results file (adv-world.result) found!\n";
        oss << "  Actual events occurred: " << actualResults_.size() << "\n";
        errorMessage = oss.str();
        return false;
    }
    
    // Compare results
    size_t maxSize = (std::max)(actualResults_.size(), expectedResults_.size());
    
    oss << "\n========== VERIFICATION REPORT ==========\n";
    oss << "Expected events: " << expectedResults_.size() << "\n";
    oss << "Actual events:   " << actualResults_.size() << "\n";
    oss << "------------------------------------------\n";
    
    for (size_t i = 0; i < maxSize; ++i) {
        bool hasExpected = (i < expectedResults_.size());
        bool hasActual = (i < actualResults_.size());
        
        if (hasExpected && hasActual) {
            const ResultEntry& exp = expectedResults_[i];
            const ResultEntry& act = actualResults_[i];
            
            bool cycleMatch = (exp.getCycle() == act.getCycle());
            bool descMatch = (exp.getDescription() == act.getDescription());
            
            if (cycleMatch && descMatch) {
                oss << "[OK]    Cycle " << exp.getCycle() << ": " << exp.getDescription() << "\n";
            } else {
                hasError = true;
                oss << "[FAIL]  Event " << i << ":\n";
                oss << "        Expected: Cycle " << exp.getCycle() << " - " << exp.getDescription() << "\n";
                oss << "        Actual:   Cycle " << act.getCycle() << " - " << act.getDescription() << "\n";
                if (!cycleMatch) {
                    oss << "        ^ CYCLE MISMATCH!\n";
                }
                if (!descMatch) {
                    oss << "        ^ DESCRIPTION MISMATCH!\n";
                }
            }
        } else if (hasExpected && !hasActual) {
            hasError = true;
            const ResultEntry& exp = expectedResults_[i];
            oss << "[MISS]  Event " << i << " - Expected but not occurred:\n";
            oss << "        Cycle " << exp.getCycle() << ": " << exp.getDescription() << "\n";
        } else if (!hasExpected && hasActual) {
            hasError = true;
            const ResultEntry& act = actualResults_[i];
            oss << "[EXTRA] Event " << i << " - Occurred but not expected:\n";
            oss << "        Cycle " << act.getCycle() << ": " << act.getDescription() << "\n";
        }
    }
    
    oss << "------------------------------------------\n";
    
    if (hasError) {
        if (actualResults_.size() != expectedResults_.size()) {
            oss << "SUMMARY: Event count mismatch!\n";
            oss << "  Expected " << expectedResults_.size() << " events, got " << actualResults_.size() << "\n";
        } else {
            oss << "SUMMARY: Event content mismatch detected\n";
        }
        oss << "==========================================\n";
        errorMessage = oss.str();
        return false;
    }
    
    oss << "SUMMARY: All " << expectedResults_.size() << " events verified successfully!\n";
    oss << "==========================================\n";
    errorMessage = oss.str();  // Contains success report
    return true;
}

// File I/O
bool GameRecorder::writeStepsFile() {
    std::ofstream file(STEPS_FILE);
    if (!file.is_open()) {
        FileParser::reportError("Cannot create steps file: " + std::string(STEPS_FILE));
        return false;
    }

    file << "# adv-world Steps File\n";
    file << "# Format: <cycle> <TYPE> <args...>\n";
    file << "# TYPE is one of: KEY SCREEN LIFE RIDDLE ANSWER END\n";
    file << "\n";

    if (!screenFiles_.empty()) {
        file << "SCREENS";
        for (const auto& s : screenFiles_) {
            file << " " << escapeString(s);
        }
        file << "\n\n";
    }

    for (const auto& event : recordedEvents_) {
        file << event.getCycle() << " " << eventTypeToString(event.getType()) << " ";
        switch (event.getType()) {
            case GameEventType::KeyPress:
                file << event.getPlayerIndex() << " " << (int)event.getKeyPressed();
                break;
            case GameEventType::ScreenTransition:
                file << event.getPlayerIndex() << " " << event.getTargetScreen();
                break;
            case GameEventType::LifeLost:
                file << event.getPlayerIndex();
                break;
            case GameEventType::RiddleEncounter:
                file << event.getPlayerIndex() << " " << escapeString(event.getRiddleQuestion());
                break;
            case GameEventType::RiddleAnswer:
                file << event.getPlayerIndex() << " " << escapeString(event.getRiddleAnswer()) << " " << (event.isRiddleCorrect() ? 1 : 0);
                break;
            case GameEventType::GameEnd:
                file << event.getScore() << " " << (event.getIsWin() ? 1 : 0);
                break;
            default:
                break;
        }
        file << "\n";
    }
    
    file.close();
    return true;
}

bool GameRecorder::writeResultFile() {
    std::ofstream file(RESULT_FILE);
    if (!file.is_open()) {
        FileParser::reportError("Cannot create result file: " + std::string(RESULT_FILE));
        return false;
    }
    
    // Write header
    file << "# adv-world Results File\n";
    file << "# Format: CYCLE DESCRIPTION\n";
    file << "\n";
    
    // Write results
    for (const auto& result : recordedResults_) {
        file << result.getCycle() << " " << escapeString(result.getDescription()) << "\n";
    }
    
    file.close();
    return true;
}

bool GameRecorder::readStepsFile() {
    std::vector<std::string> lines = FileParser::readFileLines(STEPS_FILE);
    if (lines.empty()) {
        FileParser::reportError("Cannot read steps file or file is empty: " + std::string(STEPS_FILE));
        return false;
    }
    
    loadedEvents_.clear();
    screenFiles_.clear();
    
    for (const auto& rawLine : lines) {
        std::string line = FileParser::trim(rawLine);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string firstToken;
        iss >> firstToken;

        // Optional metadata lines
        if (firstToken == "SCREENS") {
            // Example format: SCREENS "adv-world_00.screen" "adv-world_01.screen" ...
            std::string token;
            while (iss >> token) {
                screenFiles_.push_back(unescapeString(token));
            }
            continue;
        }
        if (firstToken == "SEED") {
            // Currently unused, but allowed.
            continue;
        }

        // Typed event format: <cycle> <TYPE> <args...>
        // Examples: 10 KEY 0 100, 150 SCREEN 0 1
        if (!firstToken.empty() && std::isdigit(static_cast<unsigned char>(firstToken[0]))) {
            int cycle = FileParser::parseInt(firstToken, -1);
            if (cycle < 0) continue;

            std::string maybeType;
            if (!(iss >> maybeType)) {
                continue;
            }

            // Backward compatible legacy format: CYCLE PLAYER_INDEX KEY_CODE
            // (i.e. second token is a number)
            bool legacySecondIsNumber = !maybeType.empty() && (std::isdigit(static_cast<unsigned char>(maybeType[0])) || maybeType[0] == '-');
            if (legacySecondIsNumber) {
                int playerIndex = FileParser::parseInt(maybeType, 0);
                int keyCode = 0;
                iss >> keyCode;

                GameEvent event;
                event.setCycle(cycle);
                event.setType(GameEventType::KeyPress);
                event.setPlayerIndex(playerIndex);
                event.setKeyPressed(static_cast<char>(keyCode));
                loadedEvents_.push_back(event);
                continue;
            }

            GameEventType type = stringToEventType(maybeType);
            GameEvent event;
            event.setCycle(cycle);
            event.setType(type);

            switch (type) {
                case GameEventType::KeyPress: {
                    int playerIndex = 0;
                    int keyCode = 0;
                    iss >> playerIndex >> keyCode;
                    event.setPlayerIndex(playerIndex);
                    event.setKeyPressed(static_cast<char>(keyCode));
                    break;
                }
                case GameEventType::ScreenTransition: {
                    int playerIndex = 0;
                    int targetScreen = -1;
                    iss >> playerIndex >> targetScreen;
                    event.setPlayerIndex(playerIndex);
                    event.setTargetScreen(targetScreen);
                    break;
                }
                case GameEventType::LifeLost: {
                    int playerIndex = 0;
                    iss >> playerIndex;
                    event.setPlayerIndex(playerIndex);
                    break;
                }
                case GameEventType::RiddleEncounter: {
                    int playerIndex = 0;
                    iss >> playerIndex;
                    event.setPlayerIndex(playerIndex);
                    std::string q;
                    std::getline(iss, q);
                    q = FileParser::trim(q);
                    event.setRiddleQuestion(unescapeString(q));
                    break;
                }
                case GameEventType::RiddleAnswer: {
                    int playerIndex = 0;
                    std::string answer;
                    int correctInt = 0;
                    iss >> playerIndex >> answer >> correctInt;
                    event.setPlayerIndex(playerIndex);
                    event.setRiddleAnswer(unescapeString(answer));
                    event.setRiddleCorrect(correctInt != 0);
                    break;
                }
                case GameEventType::GameEnd: {
                    int score = 0;
                    int winInt = 0;
                    iss >> score >> winInt;
                    event.setScore(score);
                    event.setIsWin(winInt != 0);
                    break;
                }
                default:
                    break;
            }

            loadedEvents_.push_back(event);
            continue;
        }
        
        // Unknown line type - ignore
    }
    
    return !loadedEvents_.empty();
}

bool GameRecorder::readResultFile() {
    std::vector<std::string> lines = FileParser::readFileLines(RESULT_FILE);
    if (lines.empty()) {
        // Result file is optional - return true but with empty results
        return true;
    }
    
    expectedResults_.clear();
    
    for (const auto& rawLine : lines) {
        std::string line = FileParser::trim(rawLine);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        int cycle;
        iss >> cycle;
        
        std::string rest;
        std::getline(iss, rest);
        rest = FileParser::trim(rest);
        
        expectedResults_.emplace_back(cycle, unescapeString(rest));
    }
    
    return true;
}

// Helper methods
std::string GameRecorder::eventTypeToString(GameEventType type) {
    switch (type) {
        case GameEventType::KeyPress: return "KEY";
        case GameEventType::ScreenTransition: return "SCREEN";
        case GameEventType::LifeLost: return "LIFE";
        case GameEventType::RiddleEncounter: return "RIDDLE";
        case GameEventType::RiddleAnswer: return "ANSWER";
        case GameEventType::GameEnd: return "END";
        default: return "UNKNOWN";
    }
}

GameEventType GameRecorder::stringToEventType(const std::string& str) {
    if (str == "KEY") return GameEventType::KeyPress;
    if (str == "SCREEN") return GameEventType::ScreenTransition;
    if (str == "LIFE") return GameEventType::LifeLost;
    if (str == "RIDDLE") return GameEventType::RiddleEncounter;
    if (str == "ANSWER") return GameEventType::RiddleAnswer;
    if (str == "END") return GameEventType::GameEnd;
    return GameEventType::KeyPress;
}

std::string GameRecorder::escapeString(const std::string& str) {
    std::string result;
    result.reserve(str.size() + 10);
    result += '"';
    for (char c : str) {
        if (c == '"') result += "\\\"";
        else if (c == '\\') result += "\\\\";
        else if (c == '\n') result += "\\n";
        else if (c == '\r') result += "\\r";
        else result += c;
    }
    result += '"';
    return result;
}

std::string GameRecorder::unescapeString(const std::string& str) {
    if (str.empty()) return str;
    
    std::string result;
    size_t start = 0;
    size_t end = str.size();
    
    // Remove surrounding quotes
    if (str[0] == '"') start = 1;
    if (str.size() > 1 && str.back() == '"') end = str.size() - 1;
    
    result.reserve(end - start);
    for (size_t i = start; i < end; ++i) {
        if (str[i] == '\\' && i + 1 < end) {
            char next = str[i + 1];
            if (next == '"') { result += '"'; ++i; }
            else if (next == '\\') { result += '\\'; ++i; }
            else if (next == 'n') { result += '\n'; ++i; }
            else if (next == 'r') { result += '\r'; ++i; }
            else result += str[i];
        } else {
            result += str[i];
        }
    }
    return result;
}
