#pragma once
#include <vector>
#include <string>

enum class MenuAction {
    None,
    NewGame,
    Continue,
    Instructions,
    Exit
};

class Menu {
public:
    // Menu actions
    static MenuAction showStartMenu();
    static void showInstructions();
    static void showLoseScreen();
    static void showWinScreen();
    
    // Helper to draw start menu without waiting for input
    static void drawStartMenu();
    
    // UI Screen Templates (cached)
    static const std::vector<std::string>& getRiddleTemplate();
    static const std::vector<std::string>& getPauseTemplate();
    
private:
    static std::vector<std::string> loadScreen(const std::string& filename);
    static void ensureLoaded(std::vector<std::string>& cache, const std::string& filename);
};