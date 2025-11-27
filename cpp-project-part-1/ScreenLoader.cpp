#include "ScreenLoader.h"
#include "Screen.h"
#include <filesystem>
#include <fstream>
#include <windows.h>
#include <algorithm>

namespace fs = std::filesystem;

static std::vector<std::string> loadLevelFromFile(const std::string& filepath) {
    std::vector<std::string> board;
    std::ifstream inputFile(filepath);
    if (!inputFile.is_open()) {
        return board;
    }

    std::string line;
    while (std::getline(inputFile, line)) {
        board.push_back(line);
    }
    inputFile.close();

    if (!board.empty()) {
        std::string& firstLine = board[0];
        if (firstLine.size() >= 3 &&
            (unsigned char)firstLine[0] == 0xEF &&
            (unsigned char)firstLine[1] == 0xBB &&
            (unsigned char)firstLine[2] == 0xBF) {
            firstLine.erase(0, 3);
        }
    }

    // Normalize to fixed size (80x25) padding with spaces to avoid out-of-range access
    for (auto& l : board) {
        if (l.size() < Screen::MAX_X) l.append(Screen::MAX_X - l.size(), ' ');
        else if (l.size() > Screen::MAX_X) l = l.substr(0, Screen::MAX_X); // trim if longer (multi-byte chars may break alignment)
    }
    while (board.size() < Screen::MAX_Y) {
        board.push_back(std::string(Screen::MAX_X, ' '));
    }
    if (board.size() > Screen::MAX_Y) {
        board.resize(Screen::MAX_Y);
    }

    return board;
}

std::vector<Screen> ScreenLoader::loadScreensFromFiles() {
    std::vector<std::string> mapFiles;

    try {
        for (const auto& entry : fs::directory_iterator(".")) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.rfind("adv-world", 0) == 0) {
                    auto extPos = filename.find_last_of('.');
                    if (extPos != std::string::npos) {
                        std::string ext = filename.substr(extPos + 1);
                        if (ext == "screen") {
                            mapFiles.push_back(filename);
                        }
                    }
                }
            }
        }
    } catch (const fs::filesystem_error&) {
    }

    std::sort(mapFiles.begin(), mapFiles.end());

    std::vector<Screen> screens;
    for (const auto& fn : mapFiles) {
        auto level = loadLevelFromFile(fn);
        if (!level.empty()) {
            screens.emplace_back(level);
        }
    }
    return screens;
}
