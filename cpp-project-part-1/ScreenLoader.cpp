#include "ScreenLoader.h"
#include "Screen.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <windows.h>
#include <algorithm>
#include <vector>

namespace fs = std::filesystem;

static std::vector<std::wstring> loadLevelFromFile(const std::string& filepath) {
    std::vector<std::wstring> board;
    std::ifstream inputFile(filepath, std::ios::binary);
    if (!inputFile.is_open()) return board;
    std::string content((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
    inputFile.close();
    if (content.size() >= 3 && (unsigned char)content[0]==0xEF && (unsigned char)content[1]==0xBB && (unsigned char)content[2]==0xBF) content.erase(0,3);
    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();
        // UTF-8 to UTF-16
        int wlen = MultiByteToWideChar(CP_UTF8,0,line.c_str(),(int)line.size(),nullptr,0);
        std::wstring wline(wlen,0);
        MultiByteToWideChar(CP_UTF8,0,line.c_str(),(int)line.size(),&wline[0],wlen);
        board.push_back(wline);
    }
    return board;
}

std::vector<Screen> ScreenLoader::loadScreensFromFiles() {
    std::vector<std::string> mapFiles;
    try {
        for (const auto& entry : fs::directory_iterator(".")) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.rfind("adv-world",0)==0) {
                    auto extPos = filename.find_last_of('.');
                    if (extPos!=std::string::npos && filename.substr(extPos+1)=="screen") mapFiles.push_back(filename);
                }
            }
        }
    } catch(...) {}
    std::sort(mapFiles.begin(), mapFiles.end());
    std::vector<Screen> screens;
    for (auto& fn : mapFiles) {
        auto level = loadLevelFromFile(fn);
        if (!level.empty()) screens.emplace_back(level);
    }
    return screens;
}
