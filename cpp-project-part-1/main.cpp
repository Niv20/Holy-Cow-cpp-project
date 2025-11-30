#include "Game.h"
#include "Menu.h"

int main() {
    bool exitProgram = false;
    
    while (!exitProgram) {
        MenuAction action = Menu::showStartMenu();
        
        switch (action) {
            case MenuAction::NewGame: {
                Game game;
                game.run();
                
                // Check if player lost
                if (game.isGameLost()) {
                    Menu::showLoseScreen();
                }
                // Loop back to menu
                break;
            }
            
            case MenuAction::Instructions:
                Menu::showInstructions();
                // Loop back to menu
                break;
            
            case MenuAction::Exit:
                exitProgram = true;
                break;
            
            case MenuAction::Continue:
                // Future implementation
                break;
            
            case MenuAction::None:
                // Do nothing, loop back to menu
                break;
        }
    }

    return 0;
}