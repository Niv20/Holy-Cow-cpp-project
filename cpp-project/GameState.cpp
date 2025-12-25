#include "GameState.h"
#include "FileParser.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace fs = std::filesystem;

// GameState implementation
GameState::GameState() {}

// PlayerState implementation
PlayerState::PlayerState()
    : roomIdx_(-1), x_(0), y_(0), diffX_(0), diffY_(0), carried_(' ') {}

PlayerState::PlayerState(int room, int x, int y, int diffX, int diffY, char carried)
    : roomIdx_(room), x_(x), y_(y), diffX_(diffX), diffY_(diffY), carried_(carried) {}

int PlayerState::getRoomIdx() const { return roomIdx_; }
int PlayerState::getX() const { return x_; }
int PlayerState::getY() const { return y_; }
int PlayerState::getDiffX() const { return diffX_; }
int PlayerState::getDiffY() const { return diffY_; }
char PlayerState::getCarried() const { return carried_; }

void PlayerState::setRoomIdx(int v) { roomIdx_ = v; }
void PlayerState::setX(int v) { x_ = v; }
void PlayerState::setY(int v) { y_ = v; }
void PlayerState::setDiffX(int v) { diffX_ = v; }
void PlayerState::setDiffY(int v) { diffY_ = v; }
void PlayerState::setCarried(char v) { carried_ = v; }

// GameStateData implementation
GameStateData::GameStateData()
: saveName_(), timestamp_(0), visibleRoomIdx_(0), heartsCount_(3), pointsCount_(0), gameCycle_(0) {}

void GameStateData::setSaveName(const std::string& name) { saveName_ = name; }
const std::string& GameStateData::getSaveName() const { return saveName_; }
void GameStateData::setTimestamp(std::time_t t) { timestamp_ = t; }
std::time_t GameStateData::getTimestamp() const { return timestamp_; }

void GameStateData::setVisibleRoomIdx(int idx) { visibleRoomIdx_ = idx; }
int GameStateData::getVisibleRoomIdx() const { return visibleRoomIdx_; }
void GameStateData::setHeartsCount(int h) { heartsCount_ = h; }
int GameStateData::getHeartsCount() const { return heartsCount_; }
void GameStateData::setPointsCount(int p) { pointsCount_ = p; }
int GameStateData::getPointsCount() const { return pointsCount_; }
void GameStateData::setGameCycle(int c) { gameCycle_ = c; }
int GameStateData::getGameCycle() const { return gameCycle_; }

void GameStateData::addPlayerState(const PlayerState& ps) { players_.push_back(ps); }
const std::vector<PlayerState>& GameStateData::getPlayers() const { return players_; }
std::vector<PlayerState>& GameStateData::getPlayersMutable() { return players_; }
void GameStateData::setPlayers(const std::vector<PlayerState>& players) { players_ = players; }

void GameStateData::setPlayerReachedFinalRoom(const std::vector<bool>& flags) { playerReachedFinalRoom_ = flags; }
const std::vector<bool>& GameStateData::getPlayerReachedFinalRoom() const { return playerReachedFinalRoom_; }
std::vector<bool>& GameStateData::getPlayerReachedFinalRoomMutable() { return playerReachedFinalRoom_; }

void GameStateData::setScreenFiles(const std::vector<std::string>& files) { screenFiles_ = files; }
const std::vector<std::string>& GameStateData::getScreenFiles() const { return screenFiles_; }

std::map<int, std::vector<std::tuple<int, int, wchar_t>>>& GameStateData::getScreenModificationsMutable() { return screenModifications_; }
const std::map<int, std::vector<std::tuple<int, int, wchar_t>>>& GameStateData::getScreenModifications() const { return screenModifications_; }

void GameStateData::setRiddleStates(const std::vector<std::tuple<int, int, int, bool>>& states) { riddleStates_ = states; }
const std::vector<std::tuple<int, int, int, bool>>& GameStateData::getRiddleStates() const { return riddleStates_; }
std::vector<std::tuple<int, int, int, bool>>& GameStateData::getRiddleStatesMutable() { return riddleStates_; }

bool GameState::ensureSavesDirectory() {
    try {
        if (!fs::exists(SAVES_DIR)) {
            fs::create_directory(SAVES_DIR);
        }
        return true;
    } catch (...) {
        return false;
    }
}

std::string GameState::buildSavePath(const std::string& saveName) {
    return std::string(SAVES_DIR) + "/" + saveName + SAVE_EXTENSION;
}

std::string GameState::formatTimestamp(std::time_t timestamp) {
    std::tm tm_buf; localtime_s(&tm_buf, &timestamp);
    std::ostringstream oss; oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S"); return oss.str();
}

std::string GameState::generateDefaultSaveName() {
    std::time_t now = std::time(nullptr);
    std::tm tm_buf; localtime_s(&tm_buf, &now);
    std::ostringstream oss; oss << "save_" << std::put_time(&tm_buf, "%Y%m%d_%H%M%S"); return oss.str();
}

bool GameState::saveState(const GameStateData& state, const std::string& saveName) {
    if (!ensureSavesDirectory()) {
        FileParser::reportError("Cannot create saves directory");
        return false;
    }
    std::string filepath = buildSavePath(saveName);
    std::ofstream file(filepath);
    if (!file.is_open()) { FileParser::reportError("Cannot create save file: " + filepath); return false; }
    
    // Header and metadata
    file << "# Holy Cow Adventure - Save File\n";
    file << "VERSION 1\n\n";
    file << "NAME " << state.getSaveName() << "\n";
    file << "TIMESTAMP " << state.getTimestamp() << "\n\n";
    
    // Game state
    file << "ROOM " << state.getVisibleRoomIdx() << "\n";
    file << "HEARTS " << state.getHeartsCount() << "\n";
    file << "POINTS " << state.getPointsCount() << "\n";
    file << "CYCLE " << state.getGameCycle() << "\n\n";
    
    // Player states
    const auto& players = state.getPlayers();
    file << "PLAYER_COUNT " << players.size() << "\n";
    for (size_t i = 0; i < players.size(); ++i) {
        const PlayerState& ps = players[i];
        file << "PLAYER " << i << " " 
             << ps.getRoomIdx() << " "
             << ps.getX() << " " 
             << ps.getY() << " "
             << ps.getDiffX() << " " 
             << ps.getDiffY() << " "
             << (int)ps.getCarried() << "\n";
    }
    file << "\n";
    
    // Final room flags
    const auto& finalFlags = state.getPlayerReachedFinalRoom();
    file << "FINAL_FLAGS " << finalFlags.size();
    for (bool flag : finalFlags) {
        file << " " << (flag ? 1 : 0);
    }
    file << "\n\n";
    
    // Screen modifications
    const auto& screenMods = state.getScreenModifications();
    file << "SCREEN_MODS_COUNT " << screenMods.size() << "\n";
    for (const auto& kv : screenMods) {
        int roomIdx = kv.first;
        const auto& mods = kv.second;
        file << "SCREEN_MOD " << roomIdx << " " << mods.size() << "\n";
        for (const auto& tup : mods) {
            file << "  MOD " << std::get<0>(tup) << " " 
                 << std::get<1>(tup) << " " 
                 << (int)std::get<2>(tup) << "\n";
        }
    }
    file << "\n";
    
    // Riddle states
    const auto& riddleStates = state.getRiddleStates();
    file << "RIDDLE_COUNT " << riddleStates.size() << "\n";
    for (const auto& t : riddleStates) {
        file << "RIDDLE " << std::get<0>(t) << " " 
             << std::get<1>(t) << " " 
             << std::get<2>(t) << " " 
             << (std::get<3>(t) ? 1 : 0) << "\n";
    }
    
    file.close();
    return true;
}

bool GameState::loadState(const std::string& saveFilePath, GameStateData& state) {
    std::ifstream file(saveFilePath);
    if (!file.is_open()) { 
        FileParser::reportError("Cannot open save file: " + saveFilePath); 
        return false; 
    }
    
    std::string line;
    while (std::getline(file, line)) {

        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;
        
        if (keyword == "VERSION") {
            int version;
            iss >> version;
            if (version != 1) {
                FileParser::reportError("Unsupported save file version: " + std::to_string(version));
                file.close();
                return false;
            }
        }
        else if (keyword == "NAME") {
            std::string name;
            std::getline(iss >> std::ws, name);
            state.setSaveName(name);
        }
        else if (keyword == "TIMESTAMP") {
            std::time_t timestamp;
            iss >> timestamp;
            state.setTimestamp(timestamp);
        }
        else if (keyword == "ROOM") {
            int room;
            iss >> room;
            state.setVisibleRoomIdx(room);
        }
        else if (keyword == "HEARTS") {
            int hearts;
            iss >> hearts;
            state.setHeartsCount(hearts);
        }
        else if (keyword == "POINTS") {
            int points;
            iss >> points;
            state.setPointsCount(points);
        }
        else if (keyword == "CYCLE") {
            int cycle;
            iss >> cycle;
            state.setGameCycle(cycle);
        }
        else if (keyword == "PLAYER_COUNT") {
            int count;
            iss >> count;
            // Players will be added by PLAYER lines
        }
        else if (keyword == "PLAYER") {
            int idx, roomIdx, x, y, diffX, diffY, carriedInt;
            iss >> idx >> roomIdx >> x >> y >> diffX >> diffY >> carriedInt;
            PlayerState ps(roomIdx, x, y, diffX, diffY, (char)carriedInt);
            state.addPlayerState(ps);
        }
        else if (keyword == "FINAL_FLAGS") {
            int count;
            iss >> count;
            std::vector<bool> flags;
            for (int i = 0; i < count; ++i) {
                int flag;
                iss >> flag;
                flags.push_back(flag != 0);
            }
            state.setPlayerReachedFinalRoom(flags);
        }
        else if (keyword == "SCREEN_MODS_COUNT") {
            // Just informational, actual mods follow
        }
        else if (keyword == "SCREEN_MOD") {
            int roomIdx, modCount;
            iss >> roomIdx >> modCount;
            auto& mods = state.getScreenModificationsMutable()[roomIdx];
            for (int i = 0; i < modCount; ++i) {
                std::string modLine;
                if (std::getline(file, modLine)) {
                    std::istringstream modIss(modLine);
                    std::string modKeyword;
                    modIss >> modKeyword;
                    if (modKeyword == "MOD") {
                        int mx, my, mch;
                        modIss >> mx >> my >> mch;
                        mods.push_back(std::make_tuple(mx, my, (wchar_t)mch));
                    }
                }
            }
        }
        else if (keyword == "RIDDLE_COUNT") {
            // Just informational, actual riddles follow
        }
        else if (keyword == "RIDDLE") {
            int roomIdx, x, y, answered;
            iss >> roomIdx >> x >> y >> answered;
            state.getRiddleStatesMutable().push_back(
                std::make_tuple(roomIdx, x, y, answered != 0));
        }
    }
    
    file.close();
    return true;
}

std::vector<std::pair<std::string, std::string>> GameState::getAvailableSaves() {
    std::vector<std::pair<std::string, std::string>> saves;
    if (!fs::exists(SAVES_DIR)) return saves;
    try {
        for (const auto& entry : fs::directory_iterator(SAVES_DIR)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                std::string ext = entry.path().extension().string();
                if (ext == SAVE_EXTENSION) {
                    GameStateData tempState;
                    // Avoid complex parsing for listing; use filename
                    saves.push_back({entry.path().string(), filename});
                }
            }
        }
    } catch(...) {}
    return saves;
}

bool GameState::deleteSave(const std::string& saveFilePath) {
    try { return fs::remove(saveFilePath); } catch(...) { return false; }
}

std::string GameState::serializeState(const GameStateData& state) {
    std::ostringstream oss;
    oss << "# placeholder";
    return oss.str();
}

bool GameState::deserializeState(const std::string& content, GameStateData& state) {
    (void)content; (void)state; return true;
}

std::string GameState::escapeString(const std::string& str) { return str; }
std::string GameState::unescapeString(const std::string& str) { return str; }
