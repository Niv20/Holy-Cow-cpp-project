#include "DataStore.h"
#include "Screen.h"
#include "SpecialDoorsData.h"
#include <fstream>
#include <sstream>

std::vector<Screen> DataStore::loadWorldScreens() {
    return Screen::loadScreensFromFiles();
}

std::vector<std::string> DataStore::loadUiScreen(const std::string& name) {
    std::vector<std::string> lines;
    std::ifstream f(name + ".screen");
    if (!f.is_open()) return lines;
    std::string line;
    while (std::getline(f, line)) lines.push_back(line);
    f.close();
    if (!lines.empty()) {
        std::string& first = lines[0];
        if (first.size() >= 3 && (unsigned char)first[0] == 0xEF && (unsigned char)first[1] == 0xBB && (unsigned char)first[2] == 0xBF) {
            first.erase(0, 3);
        }
    }
    return lines;
}

std::string DataStore::loadSpecialDoorsConfig() {
    return std::string(SPECIAL_DOORS_CONFIG);
}
