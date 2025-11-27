#pragma once
#include <vector>
#include <string>

class Screen;

class ScreenLoader {
public:
    // Scans current directory for files matching adv-world*.screen, loads them as Screen instances.
    static std::vector<Screen> loadScreensFromFiles();
};
