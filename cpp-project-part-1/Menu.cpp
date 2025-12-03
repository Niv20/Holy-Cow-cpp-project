#include <conio.h>
#include <windows.h>
#include <string>
#include <vector>
#include <fstream>

#include "Menu.h"
#include "Screen.h"
#include "utils.h"

using std::vector;
using std::string;

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

vector<string> Menu::loadScreen(const string& filename) {
    string baseName = filename.substr(0, filename.find('.'));
    vector<string> lines;
    std::ifstream f(baseName + ".screen");
    if (!f.is_open()) return lines;
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
                    return MenuAction::Exit;
            }
        }
        Sleep(180);
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
    
    while (true) {
        if (_kbhit()) {
            _getch();
            return;
        }
        Sleep(180);
    }
}

void Menu::showLoseScreen() {

    vector<string> loseScreen = loadScreen("Lose.screen");

    if (loseScreen.empty()) {
        return;
    }

    Screen screen(loseScreen);
    cls();
    screen.draw();
    
    while (true) {
        if (_kbhit()) {
            _getch();
            return;
        }
        Sleep(180);
    }
}

void Menu::showWinScreen() {

    vector<string> winScreen = loadScreen("Win.screen");

    if (winScreen.empty()) {
        // If no Win screen, just show the final room (room 7)
        return;
    }

    Screen screen(winScreen);
    cls();
    screen.draw();
    
    while (true) {
        if (_kbhit()) {
            _getch();
            return;
        }
        Sleep(180);
    }
}