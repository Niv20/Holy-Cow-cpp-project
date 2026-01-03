#include "Game.h"
#include "FileParser.h"
#include "GameRecorder.h"
#include "utils.h"
#include <iostream>
#include <exception>
#include <string>
#include <cstring>

/*      __  __      __         ______
       / / / /___  / /_  __   / ____/___ _      __  (__)
      / /_/ / __ \/ / / / /  / /   / __ \ | /| / /--(oo)
     / __  / /_/ / / /_/ /  / /___/ /_/ / |/ |/ /   (__)
    /_/ /_/\____/_/\__, /   \____/\____/|__/|__/---||
                  /____/

                Maor Adirim & Niv Libovich               */

int main(int argc, char* argv[]) {
    try {
        // Clear any previous error state
        FileParser::clearErrors();
        
        // Parse command line arguments
        GameMode mode = parseCommandLineArgs(argc, argv);
        
        // Run the appropriate game mode
        Game::runApp(mode);
        
        // Check if any non-fatal errors occurred during execution
        if (FileParser::hasErrors()) {
            // Errors were already reported, program continued gracefully
        }
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        std::cerr << "Press Enter to exit..." << std::endl;
        std::cin.get();
        return 1;
    }
    catch (...) {
        std::cerr << "Unknown fatal error occurred." << std::endl;
        std::cerr << "Press Enter to exit..." << std::endl;
        std::cin.get();
        return 1;
    }
}