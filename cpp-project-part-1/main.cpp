#include "Game.h"
#include "Menu.h"

int main() {
    // TEMP DEBUG: skip start menu and launch game directly.
    // Original code (commented out) loops on start menu:
    /*
    bool exitProgram = false;
    while (!exitProgram) {
        MenuAction action = Menu::showStartMenu();
        switch (action) {
            case MenuAction::NewGame: {
                Game game; game.run();
                if (game.isGameLost()) { Menu::showLoseScreen(); }
                break;
            }
            case MenuAction::Instructions: Menu::showInstructions(); break;
            case MenuAction::Exit: exitProgram = true; break;
            case MenuAction::Continue: break; // Future
            case MenuAction::None: break;
        }
    }
    return 0;
    */

    Game game; // starts at room index 0 (screen 1)
    game.run();
    return 0;
}