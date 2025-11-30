#pragma once
#include <string>
#include <vector>

enum class MenuAction {
    NewGame,
    Continue,
    Instructions,
    Exit,
    None
};

class Menu {
public:
    static MenuAction showStartMenu();
    static void showInstructions();
    static void showLoseScreen();

private:
    static std::vector<std::string> loadScreen(const std::string& filename);
};
