#include <conio.h>
#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>

#include "Menu.h"
#include "Screen.h"
#include "ScreenBuffer.h"
#include "utils.h"
#include "GameState.h"

using std::vector;
using std::string;
namespace fs = std::filesystem;

// ============================================
// Static caches for UI templates
// ============================================
namespace {
    constexpr char START_MENU_NEW_GAME_KEY = '1';
    constexpr char START_MENU_CONTINUE_KEY = '2';
    constexpr char START_MENU_INSTRUCTIONS_KEY = '8';
    constexpr char START_MENU_EXIT_KEY = '9';
    constexpr DWORD MENU_POLL_DELAY_MS = 180;
    constexpr int SAVE_NAME_MAX_LENGTH = 30;
    constexpr int ESC_KEY = 27;
    constexpr int ENTER_KEY = 13;
    constexpr int BACKSPACE_KEY = 8;

    vector<string> g_riddleTemplate;
    vector<string> g_pauseTemplate;
}

// ============================================
// Private helpers
// ============================================

static fs::path getExeDir() {
    wchar_t buffer[MAX_PATH];
    DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (len == 0) return fs::current_path();
    fs::path exePath(buffer);
    return exePath.parent_path();
}

static std::ifstream tryOpenScreenFile(const std::string& baseName) {
    // Try multiple candidate directories: exe dir, its parent, current path
    std::vector<fs::path> candidates;
    fs::path exeDir = getExeDir();
    candidates.push_back(exeDir);
    candidates.push_back(exeDir.parent_path());
    candidates.push_back(fs::current_path());
    for (const auto& dir : candidates) {
        fs::path full = dir / (baseName + ".screen");
        std::ifstream f(full.string());
        if (f.is_open()) {
            return f;
        }
    }
    return std::ifstream();
}

// Print Goodbye ASCII art at top left and leave console as-is
static void printGoodbyeArt() {
    cls();
    const wchar_t* lines[] = {
        L"               \x250C\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2510",
        L"               \x2502 Goodbye! \x2502",
        L"               \x2514\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2518",
        L"              /            ",
        L"        (__)               ",
        L"'\x5C------(oo)               ",
        L"  ||    (__)               ",
        L"  ||w--||                  "
    };
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    for (int i = 0; i < (int)(sizeof(lines)/sizeof(lines[0])); ++i) {
        COORD pos{0, (SHORT)i};
        SetConsoleCursorPosition(hOut, pos);
        DWORD written; WriteConsoleW(hOut, lines[i], (DWORD)wcslen(lines[i]), &written, nullptr);
    }
}

vector<string> Menu::loadScreen(const string& filename) {
string baseName = filename.substr(0, filename.find('.'));
vector<string> lines;

std::ifstream f = tryOpenScreenFile(baseName);
if (!f.is_open()) {
    // Screen file not found - return empty silently, caller should handle gracefully
    return lines;
}
    string line;
    while (std::getline(f, line)) lines.push_back(line);
    f.close();
    // Remove UTF-8 BOM if present
    if (!lines.empty()) {
        string& first = lines[0];
        if (first.size() >= 3 && 
            (unsigned char)first[0] == 0xEF && 
            (unsigned char)first[1] == 0xBB && 
            (unsigned char)first[2] == 0xBF) {
            first.erase(0, 3);
        }
    }
    return lines;
}

void Menu::ensureLoaded(vector<string>& cache, const string& filename) {
    if (cache.empty()) {
        cache = loadScreen(filename);
    }
}

// ============================================
// UI Template Accessors (cached)
// ============================================

const vector<string>& Menu::getRiddleTemplate() {
    ensureLoaded(g_riddleTemplate, "riddle.screen");
    return g_riddleTemplate;
}

const vector<string>& Menu::getPauseTemplate() {
    ensureLoaded(g_pauseTemplate, "Pause.screen");
    return g_pauseTemplate;
}

// ============================================
// Menu Actions
// ============================================

void Menu::drawStartMenu() {
    vector<string> startScreen = loadScreen("Start.screen");
    if (startScreen.empty()) {
        std::cerr << "Error: Start.screen not found or empty. Place .screen files next to the EXE or project root." << std::endl;
        return;
    }
    Screen screen(startScreen);
    cls();
    screen.draw();
    ScreenBuffer::getInstance().flush();
}

MenuAction Menu::showStartMenu() {

    drawStartMenu();
    
    while (true) {
        if (_kbhit()) {
            char key = _getch();
            switch (key) {
                case START_MENU_NEW_GAME_KEY:
                    return MenuAction::NewGame;
                case START_MENU_CONTINUE_KEY:
                    return MenuAction::LoadSavedGame;
                case START_MENU_INSTRUCTIONS_KEY:
                    return MenuAction::Instructions;
                case START_MENU_EXIT_KEY:
                    // Exit: clear screen, print goodbye art, then exit program
                    printGoodbyeArt();
                    return MenuAction::Exit;
            }
        }
        Sleep(MENU_POLL_DELAY_MS);
    }
}

void Menu::showInstructions() {

vector<string> instructionsScreen = loadScreen("Instructions.screen");

if (instructionsScreen.empty()) {
    return;
}
    
    Screen screen(instructionsScreen);
    cls();
    screen.draw();
    ScreenBuffer::getInstance().flush();
    
    while (true) {
        if (_kbhit()) {
            (void)_getch();
            return;
        }
        Sleep(MENU_POLL_DELAY_MS);
    }
}

void Menu::showLoseScreen() {

    vector<string> loseScreen = loadScreen("Lose.screen");

    // In case the BODEK accidentally deletes the file...
    if (loseScreen.empty()) {
        return;
    }

    Screen screen(loseScreen);
    cls();
    screen.draw();
    ScreenBuffer::getInstance().flush();
    
    // Flush keyboard buffer to avoid consuming stale input
    while (_kbhit()) { (void)_getch();
    }
    
    while (true) {
        if (_kbhit()) {
            (void)_getch();
            return;
        }
        Sleep(MENU_POLL_DELAY_MS);
    }
}

void Menu::showWinScreen() {

    vector<string> winScreen = loadScreen("Win.screen");

    if (winScreen.empty()) {
        // If no Win screen, just return (the final room is the win screen)
        return;
    }

    Screen screen(winScreen);
    cls();
    screen.draw();
    ScreenBuffer::getInstance().flush();
    
    // Flush keyboard buffer to avoid consuming stale input
    while (_kbhit()) { (void)_getch();
    }
    
    while (true) {
        if (_kbhit()) {
            (void)_getch();
            return;
        }
        Sleep(MENU_POLL_DELAY_MS);
    }
}

// Show save dialog - allows user to enter save name
bool Menu::showSaveDialog(std::string& saveName) {
    cls();
    
    // Load save game screen
    vector<string> saveScreen = loadScreen("SaveGame.screen");
    if (saveScreen.empty()) {
        std::cerr << "Error: SaveGame.screen not found." << std::endl;
        return false;
    }
    
    Screen screen(saveScreen);
    screen.draw();
    ScreenBuffer::getInstance().flush();
    
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Position cursor for input (center of screen, row 16)
    int inputX = 25;  // Centered position
    int inputY = 16;  // Middle of the screen
    COORD inputPos{(SHORT)inputX, (SHORT)inputY};
    SetConsoleCursorPosition(hOut, inputPos);
    
    string input;
    bool done = false;
    
    while (!done) {
        if (_kbhit()) {
            int key = _getch();
            
            if (key == ESC_KEY) {
                return false;  // Cancelled
            }
            else if (key == ENTER_KEY) {
                done = true;
            }
            else if (key == BACKSPACE_KEY) {
                if (!input.empty()) {
                    input.pop_back();
                    // Redraw input area (clear and rewrite)
                    SetConsoleCursorPosition(hOut, inputPos);
                    for (int i = 0; i < SAVE_NAME_MAX_LENGTH; ++i) {
                        std::cout << ' ';
                    }
                    SetConsoleCursorPosition(hOut, inputPos);
                    std::cout << input;
                }
            }
            else if (isprint(key) && input.size() < SAVE_NAME_MAX_LENGTH) {
                // Only allow safe characters for filenames
                if (isalnum(key) || key == '_' || key == '-' || key == ' ') {
                    input += (char)key;
                    std::cout << (char)key;
                }
            }
        }
        Sleep(50);
    }
    
    // Use default name (current date and time) if empty
    if (input.empty()) {
        // Generate default name with date and time: DD.MM.YYYY_(HH-MM) format
        // Note: Using dots and dashes because / : ; are not allowed in Windows filenames
        std::time_t now = std::time(nullptr);
        std::tm tm_buf;
        localtime_s(&tm_buf, &now);
        std::ostringstream oss;
        oss << std::put_time(&tm_buf, "%d.%m.%Y_(%H-%M)");
        saveName = oss.str();
    } else {
        // Replace spaces with underscores for filename safety
        saveName = input;
        for (char& c : saveName) {
            if (c == ' ') c = '_';
        }
    }
    
    return true;
}

// Show load dialog - displays available saves and lets user select
std::string Menu::showLoadDialog() {
    cls();
    
    // Load the load game screen
    vector<string> loadScreen = Menu::loadScreen("LoadGame.screen");
    if (loadScreen.empty()) {
        std::cerr << "Error: LoadGame.screen not found." << std::endl;
        return "";
    }
    
    Screen screen(loadScreen);
    screen.draw();
    ScreenBuffer::getInstance().flush();
    
    auto saves = GameState::getAvailableSaves();
    
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    
    if (saves.empty()) {
        // Replace the "Select a save file..." text on row 9 with "No saved games found" message
        const std::string noSavesMsg = "No saved games found. Press any key to return.";
        const int screenWidth = 78;  // Inner width (80 - 2 for the │ borders)
        const int msgRow = 9;  // Row 10 in 1-indexed (the "Select a save file..." line)
        
        // Build centered message with spaces on both sides, surrounded by │ borders
        int totalPadding = screenWidth - (int)noSavesMsg.length();
        int leftPadding = totalPadding / 2;
        int rightPadding = totalPadding - leftPadding;
        std::string centeredMsg = "\xE2\x94\x82" + std::string(leftPadding, ' ') + noSavesMsg + std::string(rightPadding, ' ') + "\xE2\x94\x82";
        
        COORD msgPos{0, (SHORT)msgRow};
        SetConsoleCursorPosition(hOut, msgPos);
        std::cout << centeredMsg;
        
        while (!_kbhit()) Sleep(100);
        (void)_getch();
        return "";
    }
    
    // Save positions: (row, column) - converted to 0-based indexing
    // 1-5 on left side, 6-9 on right side
    const int savePositions[9][2] = {
        {13, 14},  // 1
        {15, 14},  // 2
        {17, 14},  // 3
        {19, 14},  // 4
        {21, 14},  // 5
        {14, 48},  // 6
        {16, 48},  // 7
        {18, 48},  // 8
        {20, 48}   // 9
    };
    
    // Display save names at their positions
    int displayCount = (saves.size() > 9) ? 9 : (int)saves.size();
    for (int i = 0; i < displayCount; ++i) {
        int row = savePositions[i][0];
        int col = savePositions[i][1];
        
        COORD pos{(SHORT)col, (SHORT)row};
        SetConsoleCursorPosition(hOut, pos);
        
        // Get display name (filename without path and extension)
        std::string displayName = saves[i].second;
        // Remove .sav extension if present
        size_t extPos = displayName.rfind(".sav");
        if (extPos != std::string::npos) {
            displayName = displayName.substr(0, extPos);
        }
        
        // Truncate if too long (max ~25 chars to fit in the space)
        if (displayName.size() > 25) {
            displayName = displayName.substr(0, 22) + "...";
        }
        
        std::cout << displayName;
    }
    
    // Wait for selection
    while (true) {
        if (_kbhit()) {
            int key = _getch();
            
            if (key == ESC_KEY) {
                return "";  // Cancelled
            }
            
            if (key >= '1' && key <= '9') {
                int idx = key - '1';
                if (idx < displayCount) {
                    return saves[idx].first;  // Return file path
                }
            }
        }
        Sleep(100);
    }
}