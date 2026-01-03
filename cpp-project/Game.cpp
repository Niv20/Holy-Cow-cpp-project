#include <conio.h>
#include <windows.h>
#include <queue>
#include <set>
#include <vector>
#include <string>
#include <utility>
#include <iostream>
#include <filesystem>

#include "Game.h"
#include "Board.h"
#include "ScreenBuffer.h"
#include "utils.h"
#include "Glyph.h"
#include "Menu.h"
#include "RoomConnections.h"
#include "Player.h"
#include "Point.h"
#include "Riddle.h"
#include "Obstacle.h"
#include "FileParser.h"
#include "DarkRoom.h"
#include "GameRecorder.h"
#include "GameState.h"

using std::vector;
using std::string;
using std::queue;
using std::pair;
using std::set;
namespace fs = std::filesystem;

namespace {
    constexpr char PAUSE_EXIT_KEY_UPPER = 'H';
    constexpr char PAUSE_EXIT_KEY_LOWER = 'h';
    constexpr char PAUSE_SAVE_KEY_UPPER = 'S';
    constexpr char PAUSE_SAVE_KEY_LOWER = 's';
    constexpr wchar_t OVERLAP_ICON = L'O';
    constexpr char NO_INVENTORY_ITEM = ' ';
    constexpr char TORCH_CHAR = static_cast<char>(Glyph::Torch);
    
    // Tick delays for different modes
    constexpr int TICK_DELAY_NORMAL = 90;   // Normal play
    constexpr int TICK_DELAY_LOAD = 10;     // Load mode (visual playback) - faster
    constexpr int TICK_DELAY_SILENT = 0;    // Silent mode - as fast as possible
}


/*      (__)
'\------(oo)    Constructor
  ||    (__)
  ||w--||                */

Game::Game() : visibleRoomIdx(0), isRunning(true), gameMode(GameMode::Normal), gameCycle(0) { 
    initGame(); 
}

Game::Game(GameMode mode) : visibleRoomIdx(0), isRunning(true), gameMode(mode), gameCycle(0), inPauseMenu(false) { 
    initGame(); 
    
    // Initialize recorder for save/load modes
    if (mode == GameMode::Save) {
        recorder = std::make_unique<GameRecorder>();
        recorder->initForSave(loadedScreenFiles);
    }
    else if (mode == GameMode::Load || mode == GameMode::LoadSilent) {
        recorder = std::make_unique<GameRecorder>();
        if (!recorder->initForLoad()) {
            FileParser::reportError("Failed to load game recording files");
            isRunning = false;
        }
    }
}

Game::Game(const GameStateData& savedState, GameMode mode) 
    : visibleRoomIdx(0), isRunning(true), gameMode(mode), gameCycle(0), inPauseMenu(false) { 
    initGame(savedState);
}

void Game::initGame() {

world = Screen::loadScreensFromFiles();
 
if (world.empty()) { 
    FileParser::reportError("Cannot start game: No level screens could be loaded.");
    std::cerr << "Please ensure adv-world_*.screen files are present next to the executable." << std::endl;
    isRunning = false; 
    return; 
}
    
// Capture original state of all screens for tracking modifications
for (auto& screen : world) {
    screen.captureOriginalState();
}
    
// Store screen file names for recording
    try {
        fs::path baseDir = fs::current_path();
        for (const auto& entry : fs::directory_iterator(baseDir)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.rfind("adv-world", 0) == 0 && 
                    filename.find(".screen") != std::string::npos) {
                    loadedScreenFiles.push_back(filename);
                }
            }
        }
    } catch (...) {
        // Ignore directory scanning errors
    }

    // Load room connections from screen metadata
    roomConnections.loadFromScreens(world);

    Screen::scanAllScreens(world, roomConnections, riddlesByPosition, legend);

    players.push_back(Player(Point(53, 19), "wdxase", Glyph::First_Player, 0));
    players.push_back(Player(Point(63, 19), "ilmjko", Glyph::Second_Player, 0));

    // Initialize final room tracking for both players
    playerReachedFinalRoom.resize(players.size(), false);
    finalRoomFocusTicks = 0;
}

void Game::initGame(const GameStateData& savedState) {
    // First do normal initialization
    initGame();
    
    if (!isRunning) return;
    
    // Then apply saved state
    visibleRoomIdx = savedState.getVisibleRoomIdx();
    heartsCount = savedState.getHeartsCount();
    pointsCount = savedState.getPointsCount();
    gameCycle = savedState.getGameCycle();
    
    // Restore player states
    const auto& savedPlayers = savedState.getPlayers();
    for (size_t i = 0; i < savedPlayers.size() && i < players.size(); ++i) {
        const PlayerState& ps = savedPlayers[i];
        players[i].setRoomIdx(ps.getRoomIdx());
        players[i].setPosition(Point(ps.getX(), ps.getY()));
        players[i].getPosition().setDiffX(ps.getDiffX());
        players[i].getPosition().setDiffY(ps.getDiffY());
        players[i].setCarried(ps.getCarried());
    }
    
    // Restore final room flags
    playerReachedFinalRoom = savedState.getPlayerReachedFinalRoom();
    if (playerReachedFinalRoom.size() < players.size()) {
        playerReachedFinalRoom.resize(players.size(), false);
    }
    
    // Apply screen modifications
    const auto& screenMods = savedState.getScreenModifications();
    for (const auto& kv : screenMods) {
        int roomIdx = kv.first;
        const auto& mods = kv.second;
        if (roomIdx >= 0 && roomIdx < (int)world.size()) {
            for (const auto& tup : mods) {
                int x = std::get<0>(tup);
                int y = std::get<1>(tup);
                wchar_t ch = std::get<2>(tup);
                world[roomIdx].setCharAt(Point(x, y), ch);
            }
        }
    }
    
    // Apply riddle states (mark answered riddles)
    const auto& riddleStates = savedState.getRiddleStates();
    for (const auto& t : riddleStates) {
        int roomIdx = std::get<0>(t);
        int x = std::get<1>(t);
        int y = std::get<2>(t);
        bool answered = std::get<3>(t);
        if (answered) {
            // Remove the riddle from the screen
            if (roomIdx >= 0 && roomIdx < (int)world.size()) {
                world[roomIdx].setCharAt(Point(x, y), Glyph::Empty);
            }
        }
    }
}

/*      (__)
'\------(oo)    Start the application
  ||    (__)
  ||w--||                           */

void Game::runApp(GameMode mode) {

// Initialize console settings once at the start of the application
try {
    SetConsoleOutputCP(65001); 
    setConsoleFont(); 
    if (mode != GameMode::LoadSilent) {
        hideCursor();
    }
} catch (...) {
    FileParser::reportError("Warning: Could not initialize console settings");
}

// Handle load mode - run directly without menu
if (mode == GameMode::Load || mode == GameMode::LoadSilent) {
    Game game(mode);
    if (game.isRunning) {
        game.start();
    }
    return;
}

bool exitProgram = false;

while (!exitProgram) {

    MenuAction action = Menu::showStartMenu();
        switch (action) {

            case MenuAction::NewGame: {
                Game game(mode);  // Pass mode (Normal or Save)
                if (!game.isRunning) {
                    // Game failed to initialize, show error and return to menu
                    std::cerr << "Press any key to return to menu..." << std::endl;
                    (void)_getch();
                } else {
                    game.start();
                }
                break;
            }
            
            case MenuAction::LoadSavedGame: {
                std::string saveFilePath = Menu::showLoadDialog();
                if (!saveFilePath.empty()) {
                    GameStateData savedState;
                    GameState stateLoader;
                    if (stateLoader.loadState(saveFilePath, savedState)) {
                        Game game(savedState, mode);
                        if (game.isRunning) {
                            game.start();
                        }
                    } else {
                        std::cerr << "Failed to load saved game. Press any key..." << std::endl;
                        (void)_getch();
                    }
                }
                break;
            }

            case MenuAction::Instructions: {
                Menu::showInstructions();
                break;
            }

            case MenuAction::Exit: {
                exitProgram = true;
                break;
            }

            case MenuAction::Continue: break;
            case MenuAction::None: break;
        }
    }
}

void Game::start() {

if (!isRunning) return;

bool isSilent = (gameMode == GameMode::LoadSilent);

// Get the correct message lines
std::string line1, line2, line3;
DarkRoomManager::getDarkRoomMessage(world[visibleRoomIdx], players, visibleRoomIdx, line1, line2, line3);

// Render message box content from metadata (if exists)
if (!isSilent) {
    world[visibleRoomIdx].renderMessageBox(line1, line2, line3);

    // Use darkness-aware drawing if room has dark zones
    if (DarkRoomManager::roomHasDarkness(world[visibleRoomIdx])) {
        DarkRoomManager::drawWithDarkness(world[visibleRoomIdx], players, visibleRoomIdx);
    } else {
        world[visibleRoomIdx].draw();
    }
    refreshLegend(); 
    drawPlayers();
    ScreenBuffer::getInstance().flush();  // Single flush after all drawing
}

// Determine tick delay based on mode
int tickDelay = TICK_DELAY_NORMAL;
if (gameMode == GameMode::Load) {
    tickDelay = TICK_DELAY_LOAD;
}
else if (gameMode == GameMode::LoadSilent) {
    tickDelay = TICK_DELAY_SILENT;  // No delay in silent mode
}

    while (isRunning) { 
        // Handle input based on mode
        if (gameMode == GameMode::Load || gameMode == GameMode::LoadSilent) {
            handleInputFromRecorder();
        } else {
            handleInput();
        }
        
        if (!isRunning) break;
        update(); 
        
        gameCycle++;  // Increment game cycle
        
        if (tickDelay > 0) {
            Sleep(tickDelay); 
        }
    }
    
    // In Load/LoadSilent mode, we might have pending events (like GameEnd) that correspond 
    // to the state after the loop broke (e.g. due to death). Process them now.
    // Also, if the loop broke because isRunning became false inside handleInputFromRecorder (due to no more events),
    // we might still want to verify results one last time or ensure we didn't miss the very last state update.
    
    // Record game end if in save mode
    if (recorder && gameMode == GameMode::Save) {
        bool allPlayersWon = true;
        for (bool reached : playerReachedFinalRoom) {
            if (!reached) {
                allPlayersWon = false;
                break;
            }
        }
        recordGameEnd(heartsCount > 0 && allPlayersWon);
        recorder->finalizeRecording();
    }
    
    // In silent mode, verify results
    if (gameMode == GameMode::LoadSilent && recorder) {
        // If we stopped because we ran out of events, we might need to record the final state (GameEnd)
        // if it wasn't explicitly in the steps file (which it isn't anymore).
        // The game loop stops when isRunning becomes false.
        // If it stopped because of handleInputFromRecorder returning false, we are done with steps.
        // But we might have missed recording the GameEnd event if the game logic didn't trigger it yet.
        
        // Check if game end condition is met
        bool allPlayersWon = true;
        for (bool reached : playerReachedFinalRoom) {
            if (!reached) {
                allPlayersWon = false;
                break;
            }
        }
        
        // If game ended naturally (win or lose), record it
        if (heartsCount <= 0 || allPlayersWon) {
             recordGameEnd(heartsCount > 0 && allPlayersWon);
        }
        
        std::string verificationReport;
        bool passed = recorder->verifyResults(verificationReport);
        
        // Print verification report
        std::cout << verificationReport;
        
        if (passed) {
            std::cout << "\n*** TEST PASSED ***\n" << std::endl;
        } else {
            std::cout << "\n*** TEST FAILED ***\n" << std::endl;
        }
        return;  // Don't show win/lose screens in silent mode
    }
    
    if (!isSilent) {
        cls();
        
        if (heartsCount <= 0) {
            Menu::showLoseScreen();
        }
        else {
            // Check if both players reached final room (win condition)
            bool allPlayersWon = true;
            for (bool reached : playerReachedFinalRoom) {
                if (!reached) {
                    allPlayersWon = false;
                    break;
                }
            }
            
            if (allPlayersWon) {
                Menu::showWinScreen();
            }
        }
    }
    
    for (auto& pair : riddlesByPosition) delete pair.second;
}


void Game::handlePause() {

    const vector<string>& pauseTemplate = Menu::getPauseTemplate();

    if (pauseTemplate.empty()) {
        isRunning = false;
        return;
    }

    Screen pauseScreen(pauseTemplate);

    cls();
    pauseScreen.draw();
    ScreenBuffer::getInstance().flush();

    // Record ESC key press to enter pause menu
    if (recorder && gameMode == GameMode::Save) {
        recorder->recordKeyPress(gameCycle, 0, ESC_KEY);
    }

    while (true) {

        if (_kbhit()) {

            char key = _getch();

            if (key == ESC_KEY) {
                // Record ESC key press to exit pause menu
                if (recorder && gameMode == GameMode::Save) {
                    recorder->recordKeyPress(gameCycle, 0, ESC_KEY);
                }
                // Use drawEverything to properly handle darkness rendering
                drawEverything();
                return;
            }
            else if (key == 'H' || key == 'h') {
                // Record H/h key press to exit game
                if (recorder && gameMode == GameMode::Save) {
                    recorder->recordKeyPress(gameCycle, 0, key);
                }
                cls();
                isRunning = false;
                return;
            }
            else if (key == 'S' || key == 's') {
                // If S is pressed, we handle save state.
                handleSaveState();
                // Redraw pause screen after save
                cls();
                pauseScreen.draw();
                ScreenBuffer::getInstance().flush();
            }
        }

        Sleep(GAME_TICK_DELAY_MS);
    }
}

void Game::handleSaveState() {
    std::string saveName;
    if (Menu::showSaveDialog(saveName)) {
        GameStateData state = captureState();
        state.setSaveName(saveName);
        
        GameState saver;
        if (saver.saveState(state, saveName)) {
            // Show success message briefly
            cls();
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            COORD pos{30, 12};
            SetConsoleCursorPosition(hOut, pos);
            std::cout << "Game saved successfully!";
            Sleep(1500);
        } else {
            cls();
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            COORD pos{30, 12};
            SetConsoleCursorPosition(hOut, pos);
            std::cout << "Failed to save game!";
            Sleep(1500);
        }
    }
}

GameStateData Game::captureState() const {
    GameStateData state;
    
    state.setTimestamp(std::time(nullptr));
    state.setVisibleRoomIdx(visibleRoomIdx);
    state.setHeartsCount(heartsCount);
    state.setPointsCount(pointsCount);
    state.setGameCycle(gameCycle);
    state.setScreenFiles(loadedScreenFiles);
    state.setPlayerReachedFinalRoom(playerReachedFinalRoom);
    
    // Capture player states
    for (const auto& p : players) {
        PlayerState ps;
        ps.setRoomIdx(p.getRoomIdx());
        ps.setX(p.getPosition().getX());
        ps.setY(p.getPosition().getY());
        ps.setDiffX(p.getPosition().getDiffX());
        ps.setDiffY(p.getPosition().getDiffY());
        ps.setCarried(p.getCarried());
        state.addPlayerState(ps);
    }
    
    // Capture screen modifications (cells that changed from original)
    for (size_t roomIdx = 0; roomIdx < world.size(); ++roomIdx) {
        auto mods = world[roomIdx].getModifications();
        if (!mods.empty()) {
            state.getScreenModificationsMutable()[(int)roomIdx] = mods;
        }
    }
    
    return state;
}


void Game::refreshLegend() {
    char p1Inv = players.size() > 0 ? players[0].getCarried() : NO_INVENTORY_ITEM;
    char p2Inv = players.size() > 1 ? players[1].getCarried() : NO_INVENTORY_ITEM;
    legend.drawLegend(visibleRoomIdx, heartsCount, pointsCount, p1Inv, p2Inv);
}

void Game::drawPlayers() {
    ScreenBuffer& buffer = ScreenBuffer::getInstance();
    
    // Collect positions of players in current room
    std::vector<std::pair<Point, size_t>> playerPositions;
    for (size_t i = 0; i < players.size(); ++i) {
        if (players[i].getRoomIdx() == visibleRoomIdx) {
            playerPositions.push_back({players[i].getPosition(), i});
        }
    }
    
    // Draw players, handling overlapping positions
    std::set<std::pair<int,int>> drawnPositions;
    for (const auto& [pos, idx] : playerPositions) {
        auto posKey = std::make_pair(pos.getX(), pos.getY());
        if (drawnPositions.count(posKey) == 0) {
            // Check if another player is at the same position
            bool overlapping = false;
            for (const auto& [otherPos, otherIdx] : playerPositions) {
                if (otherIdx != idx && otherPos.getX() == pos.getX() && otherPos.getY() == pos.getY()) {
                    overlapping = true;
                    break;
                }
            }
            
            if (overlapping) {
                // Draw a combined symbol when players overlap
                buffer.setChar(pos.getX(), pos.getY(), OVERLAP_ICON);
            } else {
                players[idx].draw();
            }
            drawnPositions.insert(posKey);
        }
    }
}

void Game::handleInput() {

    if (_kbhit()) {

        char key = _getch();

        // ESC opens pause menu
        if (key == ESC_KEY) { 
            handlePause(); 
            return; 
        }

        // If both players reached final room, any key returns to start menu
        bool allAtFinal = true;
        for (size_t i = 0; i < players.size(); i++) {
            if (players[i].getRoomIdx() != FINAL_ROOM_INDEX) { allAtFinal = false; break; }
        }
        if (allAtFinal) {
            // End game loop immediately
            isRunning = false;
            return;
        }

        // Try key on each player - only record if at least one player found it meaningful
        bool anyMeaningful = false;
        for (size_t i = 0; i < players.size(); i++) {
            auto& p = players[i];
            if (p.getRoomIdx() == visibleRoomIdx && !hasPlayerReachedFinalRoom(i)) {
                bool meaningful = p.handleKey(key);
                if (meaningful) {
                    anyMeaningful = true;
                    
                    // Record key press if in save mode and key was meaningful
                    if (recorder && gameMode == GameMode::Save) {
                        recorder->recordKeyPress(gameCycle, (int)i, key);
                    }
                }
            }
        }
    }
}

void Game::handleInputFromRecorder() {
    if (!recorder) {
        isRunning = false;
        return;
    }

    // Check if we have any events left to process.
    if (!recorder->hasNextEvent()) {
        isRunning = false;
        return;
    }
    
    // Process all events for the current cycle
    while (recorder->shouldProcessEvent(gameCycle)) {
        const GameEvent& event = recorder->peekNextEvent();
        
        switch (event.getType()) {
            case GameEventType::KeyPress: {
                char key = event.getKeyPressed();
                
                // Handle Pause Menu logic during playback
                if (inPauseMenu) {
                    recorder->consumeNextEvent();
                    if (key == ESC_KEY) {
                        // Exit pause menu
                        inPauseMenu = false;
                    } else if (key == 'H' || key == 'h') {
                        // Exit game from pause menu
                        isRunning = false;
                        // Record game end if needed (though usually H means quit without win/lose logic, 
                        // but user said "Game ended: LOSE with score 0" should appear in result)
                        // If the user quits, it might be considered a loss or just end.
                        // The user's example shows "Game ended: LOSE with score 0".
                        // So we should probably treat it as a loss or just record the end state.
                        if (gameMode == GameMode::LoadSilent) {
                             std::ostringstream oss;
                             // If quitting via H, it's usually not a standard win/lose, but let's follow the user's implication
                             // that it produces a result entry.
                             // If hearts > 0, it's just an exit. But maybe the user implies it counts as a loss?
                             // Or maybe the user just wants the "Game ended" message.
                             // Let's use the standard recordGameEnd logic which checks hearts/win condition.
                             // If we quit, we haven't won.
                             recordGameEnd(false); 
                        }
                    }
                    // Ignore other keys in pause menu during playback (like 'S')
                    break;
                }

                // Enter Pause Menu
                if (key == ESC_KEY) {
                    recorder->consumeNextEvent();
                    inPauseMenu = true;
                    break;
                }
                
                // Riddle answer keys (1-4) should NOT be processed here.
                // They will be handled by Riddle::handleEncounter when the player is on a riddle.
                // Just skip riddle answer keys here - they'll be consumed by Riddle::handleEncounter.
                bool isRiddleAnswerKey = (key >= '1' && key <= '4');
                if (isRiddleAnswerKey) {
                    // Don't consume here - let Riddle::handleEncounter handle it
                    return;
                }
                    
                // Consume and apply key
                recorder->consumeNextEvent();
                
                // Apply to all players (each will check if it's their key)
                for (size_t i = 0; i < players.size(); ++i) {
                    players[i].handleKey(key);
                }
                break;
            }
            
            default:
                recorder->consumeNextEvent();
                break;
        }
    }
}




// Recording helper methods
void Game::recordScreenTransition(int playerIndex, int targetScreen) {
    if (recorder) {
        if (gameMode == GameMode::Save) {
            recorder->recordScreenTransition(gameCycle, playerIndex, targetScreen);
        } else if (gameMode == GameMode::LoadSilent) {
            std::ostringstream oss;
            oss << "Player " << (playerIndex + 1) << " moved to screen " << targetScreen;
            recorder->addActualResult(gameCycle, oss.str());
        }
    }
}

void Game::recordLifeLost(int playerIndex) {
    if (recorder) {
        if (gameMode == GameMode::Save) {
            recorder->recordLifeLost(gameCycle, playerIndex);
        } else if (gameMode == GameMode::LoadSilent) {
            std::ostringstream oss;
            oss << "Player " << (playerIndex + 1) << " lost a life";
            recorder->addActualResult(gameCycle, oss.str());
        }
    }
}

void Game::recordRiddleEvent(int playerIndex, const std::string& question, const std::string& answer, bool correct) {
    if (recorder) {
        if (gameMode == GameMode::Save) {
            recorder->recordRiddleEncounter(gameCycle, playerIndex, question);
            recorder->recordRiddleAnswer(gameCycle, playerIndex, answer, correct);
        }
    }
}

void Game::recordGameEnd(bool isWin) {
    if (recorder) {
        if (gameMode == GameMode::Save) {
            recorder->recordGameEnd(gameCycle, pointsCount, isWin);
        } else if (gameMode == GameMode::LoadSilent) {
            std::ostringstream oss;
            oss << "Game ended: " << (isWin ? "WIN" : "LOSE") << " with score " << pointsCount;
            recorder->addActualResult(gameCycle, oss.str());
        }
    }
}

void Game::update() {

if (heartsCount <= 0) { 
    isRunning = false; 
    return; 
}

bool isSilent = (gameMode == GameMode::LoadSilent);

// Track player state before movement (room, position, carried) to detect relevant changes
struct PlayerSnapshot {
    int roomIdx;
    Point pos;
    char carried;
};
std::vector<PlayerSnapshot> beforeSnapshots;
beforeSnapshots.reserve(players.size());
for (const auto& p : players) {
    beforeSnapshots.push_back({ p.getRoomIdx(), p.getPosition(), p.getCarried() });
}

// Track player rooms before movement to detect teleportation
std::vector<int> roomsBefore;
for (const auto& p : players) {
    roomsBefore.push_back(p.getRoomIdx());
}

// Store previous positions for darkness update optimization
previousPlayerPositions.clear();
for (const auto& p : players) {
    if (p.getRoomIdx() == visibleRoomIdx) {
        previousPlayerPositions.push_back(p.getPosition());
    }
}

// Move all players
for (size_t i = 0; i < players.size(); ++i) {
    auto& p = players[i];

    if (p.getRoomIdx() == visibleRoomIdx) {
        p.move(world[visibleRoomIdx], *this);
    }

    // Always resolve riddle encounters against the player's actual room.
    // Camera focus (`visibleRoomIdx`) can differ from the player's room (e.g. final-room focus),
    // and using `visibleRoomIdx` here can cause clearing the wrong screen.
    const int playerRoomIdx = p.getRoomIdx();
    if (playerRoomIdx >= 0 && playerRoomIdx < (int)world.size()) {
        Point pos = p.getPosition();
        wchar_t cell = world[playerRoomIdx].getCharAt(pos);
        if (Glyph::isRiddle(cell)) {
            Riddle::handleEncounter(p, riddlesByPosition, *this);
        }
    }
}

// Update pressure buttons after all movement to reflect new player positions
updatePressureButtons();

// Update darkness when players in room move or when torch state changes (carry/pick/drop or torch carrier enters/leaves)
if (DarkRoomManager::roomHasDarkness(world[visibleRoomIdx])) {
    bool needsDarkUpdate = false;
    bool torchChange = false;

    for (size_t i = 0; i < players.size(); i++) {
        const auto& before = beforeSnapshots[i];
        const auto& after = players[i];

        bool beforeInRoom = before.roomIdx == visibleRoomIdx;
        bool afterInRoom = after.getRoomIdx() == visibleRoomIdx;
        bool beforeTorch = before.carried == TORCH_CHAR;
        bool afterTorch = after.getCarried() == TORCH_CHAR;

        // Any movement of a player currently in the room requires redraw (players are drawn via darkness layer)
        if (beforeInRoom || afterInRoom) {
            if (before.pos.getX() != after.getPosition().getX() || before.pos.getY() != after.getPosition().getY() || beforeInRoom != afterInRoom) {
                needsDarkUpdate = true;
            }
        }

        // Torch carrier entered or left the visible room
        if (beforeInRoom != afterInRoom && (beforeTorch || afterTorch)) {
            torchChange = true;
        }

        // Torch picked up or dropped, or torch moved
        if ((beforeTorch != afterTorch) || ((beforeTorch || afterTorch) && (before.pos.getX() != after.getPosition().getX() || before.pos.getY() != after.getPosition().getY()))) {
            torchChange = true;
        }
    }

    if (needsDarkUpdate) {
        // Collect dropped torch positions so they contribute light/halo
        std::vector<Point> torchSources;
        if (torchChange) {
            for (int y = 0; y < Screen::MAX_Y; ++y) {
                for (int x = 0; x < Screen::MAX_X; ++x) {
                    Point p{x, y};
                    if (Glyph::isTorch(world[visibleRoomIdx].getCharAt(p))) {
                        torchSources.push_back(p);
                    }
                }
            }
        }

        if (!isSilent) {
            DarkRoomManager::updateDarknessAroundPlayers(world[visibleRoomIdx], players,
                                                         visibleRoomIdx, previousPlayerPositions, torchSources);
        }
    }
}
    
// Check for teleportation (room changed without edge transition)
for (size_t i = 0; i < players.size(); ++i) {
    int roomBefore = roomsBefore[i];
    int roomAfter = players[i].getRoomIdx();
    if (roomBefore != roomAfter && roomBefore == visibleRoomIdx) {
        // Player teleported to a different room - update camera
        bool anyPlayerInCurrentRoom = false;
        for (const auto& p : players) {
            if (p.getRoomIdx() == visibleRoomIdx) {
                anyPlayerInCurrentRoom = true;
                break;
            }
        }
        if (!anyPlayerInCurrentRoom) {
            visibleRoomIdx = roomAfter;
            if (!isSilent) {
                drawEverything();
            }
        }
    }
}
    
// Draw players only if room doesn't have darkness (darkness update includes players)
if (!isSilent && !DarkRoomManager::roomHasDarkness(world[visibleRoomIdx])) {
    drawPlayers();
}
    
    SpecialDoor::updateAll(*this);
    checkAndProcessTransitions(); 
    
    // Update final room tracking AFTER transitions
    for (size_t i = 0; i < players.size(); ++i) {
        if (players[i].getRoomIdx() == FINAL_ROOM_INDEX) {
            if (!playerReachedFinalRoom[i]) {
                playerReachedFinalRoom[i] = true;
                
                // Player just entered final room
                // Focus on final room for a brief period
                visibleRoomIdx = FINAL_ROOM_INDEX;
                finalRoomFocusTicks = FINAL_ROOM_FOCUS_TICKS;
                if (!isSilent) {
                    cls();
                    world[visibleRoomIdx].draw();
                    refreshLegend();
                    drawPlayers();
                    ScreenBuffer::getInstance().flush();
                }
            }
        }
    }
    
    // If focusing on final room, count down and then switch to other player if exists
    if (finalRoomFocusTicks > 0) {
        finalRoomFocusTicks--;
        if (finalRoomFocusTicks == 0) {
            // Switch camera to the room where any non-final player is
            for (size_t j = 0; j < players.size(); ++j) {
                if (players[j].getRoomIdx() != FINAL_ROOM_INDEX) {
                    visibleRoomIdx = players[j].getRoomIdx();
                    if (!isSilent) {
                        cls();
                        world[visibleRoomIdx].draw();
                        refreshLegend();
                        drawPlayers();
                        ScreenBuffer::getInstance().flush();
                    }
                    break;
                }
            }
        }
    }
    
    // Check if both players reached final room - any key will now exit to start
    bool allPlayersReachedFinal = true;
    for (size_t i = 0; i < players.size(); ++i) {
        if (players[i].getRoomIdx() != FINAL_ROOM_INDEX) {
            allPlayersReachedFinal = false;
            break;
        }
    }
    
    if (allPlayersReachedFinal) {
            // Stop all updates; handleInput will catch key and exit
            Bomb::tickAndHandleAll(bombs, *this);
            if (!isSilent) {
                refreshLegend(); 
                if (!DarkRoomManager::roomHasDarkness(world[visibleRoomIdx])) {
                    drawPlayers();
                }
                ScreenBuffer::getInstance().flush();
            }
            return;
        }
    
        Bomb::tickAndHandleAll(bombs, *this);
        if (!isSilent) {
            refreshLegend(); 
            if (!DarkRoomManager::roomHasDarkness(world[visibleRoomIdx])) {
                drawPlayers();
            }
            ScreenBuffer::getInstance().flush();  // Single flush at end of update
        }
    }

void Game::updatePressureButtons() {
    bool isSilent = (gameMode == GameMode::LoadSilent);
    for (size_t roomIdx = 0; roomIdx < world.size(); ++roomIdx) {
        Screen& screen = world[roomIdx];
        auto& buttons = screen.getDataMutable().pressureButtons;
        if (buttons.empty()) continue;

        struct TargetState {
            wchar_t original = Glyph::Empty;
            bool hasOriginal = false;
            int activeCount = 0;
        };

        std::map<std::pair<int, int>, TargetState> targets;

        std::vector<Point> roomPlayers;
        for (const auto& pl : players) {
            if (pl.getRoomIdx() == (int)roomIdx) {
                roomPlayers.push_back(pl.getPosition());
            }
        }

        for (auto& ps : buttons) {
            bool active = false;
            Point psPos = ps.getPos();
            for (const auto& pp : roomPlayers) {
                if (pp.getX() == psPos.getX() && pp.getY() == psPos.getY()) {
                    active = true;
                    break;
                }
            }

            for (const auto& tgt : ps.getTargets()) {
                Point tgtPos = tgt.getPos();
                auto key = std::make_pair(tgtPos.getX(), tgtPos.getY());
                auto& state = targets[key];
                if (!state.hasOriginal) {
                    state.original = tgt.getOriginalChar();
                    state.hasOriginal = true;
                }
                if (active) {
                    state.activeCount++;
                }
            }
        }

        bool isVisibleRoom = (visibleRoomIdx == (int)roomIdx);
        bool isDark = DarkRoomManager::roomHasDarkness(screen);

        for (auto& kv : targets) {
            Point p{ kv.first.first, kv.first.second };
            wchar_t desired = (kv.second.activeCount > 0) ? Glyph::Empty : kv.second.original;
            if (screen.getCharAt(p) != desired) {
                screen.setCharAt(p, desired);
                if (isVisibleRoom && !isSilent) {
                    if (isDark) {
                        DarkRoomManager::refreshCellWithDarkness(screen, p, players, (int)roomIdx);
                    } else {
                        screen.refreshCell(p);
                    }
                }
            }
        }
    }
}

// Written by AI! Thanks Gemini :)
void Game::checkAndProcessTransitions() {
    const int maxX = Screen::MAX_X;
    const int maxY = Screen::MAX_Y;

    struct TransitionInfo {
        Player* player;
        Direction direction;
        int targetRoom;
        Point originalPos;
        int order;
    };

    vector<TransitionInfo> transitions;
    int order = 0;

    // Detect transitions when crossing an edge
    for (auto& p : players) {
        if (p.getRoomIdx() != visibleRoomIdx) continue;
        Point pos = p.getPosition();
        Direction dir = Direction::None;
        if (pos.getX() <= 0)
            dir = Direction::Left;
        else if (pos.getX() >= maxX - 1)
            dir = Direction::Right;
        else if (pos.getY() <= 0)
            dir = Direction::Up;
        else if (pos.getY() >= maxY - 1)
            dir = Direction::Down;

        if (dir != Direction::None) {
            int nextRoom = roomConnections.getTargetRoom(visibleRoomIdx, dir);
            if (nextRoom != -1) transitions.push_back({ &p, dir, nextRoom, pos, order++ });
        }
    }

    if (transitions.empty())
        return;

    // Erase players from current room at their original positions to prevent wrap ghosting
    for (auto& t : transitions)
        world[visibleRoomIdx].refreshCell(t.originalPos);

    // Compute spawn positions per player based on their direction and target room
    vector<Point> spawns(transitions.size());

    for (size_t i = 0; i < transitions.size(); ++i) {
        const auto& t = transitions[i];
        Point s;
        switch (t.direction) {
            case Direction::Left:  s = Point(maxX - 2, t.originalPos.getY()); break;
            case Direction::Right: s = Point(1,        t.originalPos.getY()); break;
            case Direction::Up:    s = Point(t.originalPos.getX(), maxY - 2); break;
            case Direction::Down:  s = Point(t.originalPos.getX(), 1);        break;
            default:               s = t.originalPos; break;
        }
        if (s.getX() < 1) s.setX(1); if (s.getX() > maxX - 2) s.setX(maxX - 2);
        if (s.getY() < 1) s.setY(1); if (s.getY() > maxY - 2) s.setY(maxY - 2);
        spawns[i] = s;
    }

    // Helper to check if a position is valid for spawning
    auto isValidSpawn = [&](const Point& p, int roomIdx, const vector<Point>& existingSpawns, size_t upToIdx) {
        // Check bounds
        if (p.getX() < 1 || p.getX() > maxX - 2 || p.getY() < 1 || p.getY() > maxY - 2)
            return false;
        // Check not a wall
        wchar_t ch = world[roomIdx].getCharAt(p);
        if (Glyph::isWall(ch))
            return false;
        // Check not colliding with earlier spawns in this transition batch
        for (size_t k = 0; k < upToIdx; ++k) {
            if (existingSpawns[k].getX() == p.getX() && existingSpawns[k].getY() == p.getY())
                return false;
        }
        // Check not colliding with any player already in target room
        for (const auto& pl : players) {
            if (pl.getRoomIdx() == roomIdx) {
                Point plPos = pl.getPosition();
                if (plPos.getX() == p.getX() && plPos.getY() == p.getY())
                    return false;
            }
        }
        return true;
    };

    // Ensure no two players spawn at the same position, and no spawn on existing player
    for (size_t idx = 0; idx < spawns.size(); ++idx) {
        int roomIdx = transitions[idx].targetRoom;
        Direction dir = transitions[idx].direction;
        
        // Check if current spawn is valid
        if (!isValidSpawn(spawns[idx], roomIdx, spawns, idx)) {
            // Need to find alternative position
            bool placed = false;
            
            // Try offsets in perpendicular direction first, then along entry direction
            for (int offset = 1; offset <= 10 && !placed; ++offset) {
                Point candidates[4];
                if (dir == Direction::Left || dir == Direction::Right) {
                    // Entry is horizontal, try vertical offsets first
                    candidates[0] = Point(spawns[idx].getX(), spawns[idx].getY() + offset);
                    candidates[1] = Point(spawns[idx].getX(), spawns[idx].getY() - offset);
                    candidates[2] = Point(spawns[idx].getX() + offset, spawns[idx].getY());
                    candidates[3] = Point(spawns[idx].getX() - offset, spawns[idx].getY());
                } else {
                    // Entry is vertical, try horizontal offsets first
                    candidates[0] = Point(spawns[idx].getX() + offset, spawns[idx].getY());
                    candidates[1] = Point(spawns[idx].getX() - offset, spawns[idx].getY());
                    candidates[2] = Point(spawns[idx].getX(), spawns[idx].getY() + offset);
                    candidates[3] = Point(spawns[idx].getX(), spawns[idx].getY() - offset);
                }
                
                for (int c = 0; c < 4 && !placed; ++c) {
                    if (isValidSpawn(candidates[c], roomIdx, spawns, idx)) {
                        spawns[idx] = candidates[c];
                        placed = true;
                    }
                }
            }
        }
    }

    // Apply transitions per player
    for (size_t i = 0; i < transitions.size(); ++i) {
        Player* pl = transitions[i].player;
        Point newPos = spawns[i];
        newPos.setDiffX(transitions[i].originalPos.getDiffX());
        newPos.setDiffY(transitions[i].originalPos.getDiffY());
        pl->setRoomIdx(transitions[i].targetRoom);
        pl->setPosition(newPos);
        
        // Record screen transition for result file
        int playerIdx = -1;
        for (size_t j = 0; j < players.size(); ++j) {
            if (&players[j] == pl) {
                playerIdx = (int)j;
                break;
            }
        }
        if (playerIdx >= 0) {
            recordScreenTransition(playerIdx, transitions[i].targetRoom);
        }
    }

    // Camera logic:
    bool anyPlayerRemainsInCurrentRoom = false;
    for (const auto& p : players) {
        if (p.getRoomIdx() == visibleRoomIdx) {
            anyPlayerRemainsInCurrentRoom = true;
            break;
        }
    }
    if (anyPlayerRemainsInCurrentRoom) {
        return;
    }

    int focusRoom = transitions.back().targetRoom;
    visibleRoomIdx = focusRoom;
    
    bool isSilent = (gameMode == GameMode::LoadSilent);
    if (!isSilent) {
        drawEverything();
    }
}

void Game::drawEverything() { 
    cls(); 
    
    // Get the correct message lines
    std::string line1, line2, line3;
    DarkRoomManager::getDarkRoomMessage(world[visibleRoomIdx], players, visibleRoomIdx, line1, line2, line3);

    // Render message box content from metadata (if exists)
    world[visibleRoomIdx].renderMessageBox(line1, line2, line3);
    
    // Use darkness-aware drawing if room has dark zones
    if (DarkRoomManager::roomHasDarkness(world[visibleRoomIdx])) {
        DarkRoomManager::drawWithDarkness(world[visibleRoomIdx], players, visibleRoomIdx);
    } else {
        world[visibleRoomIdx].draw();
    }
    refreshLegend(); 
    drawPlayers();
    ScreenBuffer::getInstance().flush();  // Single flush after all drawing
}

/*      (__)
'\------(oo)    Convenience Wrappers 
  ||    (__)
  ||w--||                         */


void Game::placeBomb(int roomIdx, const Point& pos, int delay) {
    Bomb::place(bombs, roomIdx, pos, delay);
}


void Game::removeBombAt(int roomIdx, const Point& pos) {
    // Remove bomb at specific position (when player picks it up)
    bombs.erase(
        std::remove_if(bombs.begin(), bombs.end(),
            [roomIdx, &pos](const Bomb& b) {
                return b.getRoomIdx() == roomIdx && 
                       b.getPosition().getX() == pos.getX() && 
                       b.getPosition().getY() == pos.getY();
            }),
        bombs.end()
    );
}

class SpringData* Game::findSpringAt(int roomIdx, const Point& p) {
    return SpringData::findAt(world[roomIdx], p);
}

class SwitchData* Game::findSwitchAt(int roomIdx, const Point& p) {
    return SwitchData::findAt(world[roomIdx], p);
}

SpecialDoor* Game::findSpecialDoorAt(int roomIdx, const Point& p) {
    return SpecialDoor::findAt(world[roomIdx], p);
}

Obstacle* Game::findObstacleAt(int roomIdx, const Point& p) {
    return Obstacle::findAt(world[roomIdx], roomIdx, p);
}

// Rescan obstacles across all rooms to keep obstacle instances in sync after moves
void Game::rescanObstacles() {
    Obstacle::scanAllObstacles(world, roomConnections);
}