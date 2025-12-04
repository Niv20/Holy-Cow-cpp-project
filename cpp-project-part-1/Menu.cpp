#include <conio.h>
#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <iostream>

#include "Menu.h"
#include "Screen.h"
#include "utils.h"

using std::vector;
using std::string;
namespace fs = std::filesystem;

// ============================================
// Static caches for UI templates
// ============================================
namespace {
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
        std::cerr << "Error: Failed to open UI screen file '" << baseName << ".screen' in any of: "
                  << getExeDir().string() << ", " << getExeDir().parent_path().string() << ", "
                  << fs::current_path().string() << std::endl;
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
}

MenuAction Menu::showStartMenu() {

    drawStartMenu();
    
    while (true) {
        if (_kbhit()) {
            char key = _getch();
            switch (key) {
                case '1':
                    return MenuAction::NewGame;
                case '2':
                    // TODO: TARGIL 2!!!!!!!!!!!!!!!!!!!!!!!!!
                    break;
                case '8':
                    return MenuAction::Instructions;
                case '9':
                    // Exit: clear screen, print goodbye art, then exit program
                    printGoodbyeArt();
                    return MenuAction::Exit;
            }
        }
        Sleep(180);
    }
}

void Menu::showInstructions() {

    vector<string> instructionsScreen = loadScreen("Instructions.screen");

    if (instructionsScreen.empty()) {
        std::cerr << "Error: Instructions.screen not found or empty." << std::endl;
        return;
    }
    
    Screen screen(instructionsScreen);
    cls();
    screen.draw();
    
    while (true) {
        if (_kbhit()) {
            (void)_getch();
            return;
        }
        Sleep(180);
    }
}

void Menu::showLoseScreen() {

    vector<string> loseScreen = loadScreen("Lose.screen");

    if (loseScreen.empty()) {
        std::cerr << "Error: Lose.screen not found or empty." << std::endl;
        return;
    }

    Screen screen(loseScreen);
    cls();
    screen.draw();
    
    while (true) {
        if (_kbhit()) {
            (void)_getch();
            return;
        }
        Sleep(180);
    }
}

void Menu::showWinScreen() {

    vector<string> winScreen = loadScreen("Win.screen");

    if (winScreen.empty()) {
        // If no Win screen, just show the final room (room 7)
        std::cerr << "Warning: Win.screen not found." << std::endl;
        return;
    }

    Screen screen(winScreen);
    cls();
    screen.draw();
    
    while (true) {
        if (_kbhit()) {
            (void)_getch();
            return;
        }
        Sleep(180);
    }
}