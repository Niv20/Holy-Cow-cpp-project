#include "Menu.h"
#include "Screen.h"
#include "utils.h"
#include <conio.h>
#include <windows.h>
#include <fstream>

using namespace std;

vector<string> Menu::loadScreen(const string& filename) {
    vector<string> lines;
    ifstream f(filename);
    if (!f.is_open()) return lines;
    
    string line;
    while (getline(f, line)) {
        lines.push_back(line);
    }
    f.close();
    
    // Remove BOM if present
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

MenuAction Menu::showStartMenu() {
    vector<string> startScreen = loadScreen("Start.screen");
    if (startScreen.empty()) return MenuAction::Exit;
    
    Screen screen(startScreen);
    cls();
    screen.draw();
    
    while (true) {
        if (_kbhit()) {
            char key = _getch();
            switch (key) {
                case '1':
                    return MenuAction::NewGame;
                case '2':
                    // Future: Continue game
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
    if (instructionsScreen.empty()) return;
    
    Screen screen(instructionsScreen);
    cls();
    screen.draw();
    
    // Wait for ESC key
    while (true) {
        if (_kbhit()) {
            char key = _getch();
            if (key == 27) { // ESC key
                return;
            }
        }
        Sleep(180);
    }
}

void Menu::showLoseScreen() {
    vector<string> loseScreen = loadScreen("Lose.screen");
    if (loseScreen.empty()) return;
    
    Screen screen(loseScreen);
    cls();
    screen.draw();
    
    // Wait for any key
    while (true) {
        if (_kbhit()) {
            _getch(); // Consume the key
            return;
        }
        Sleep(180);
    }
}
