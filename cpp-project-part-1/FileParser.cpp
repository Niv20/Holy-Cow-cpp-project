#include "FileParser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <windows.h>

namespace fs = std::filesystem;

// Static member initialization
ErrorCallback FileParser::s_errorHandler = FileParser::defaultErrorHandler;
bool FileParser::s_hasErrors = false;

void FileParser::defaultErrorHandler(const std::string& message) {
    std::cerr << "Error: " << message << std::endl;
}

void FileParser::setErrorHandler(ErrorCallback handler) {
    s_errorHandler = handler ? handler : defaultErrorHandler;
}

void FileParser::reportError(const std::string& message) {
    s_hasErrors = true;
    if (s_errorHandler) {
        s_errorHandler(message);
    }
}

bool FileParser::hasErrors() {
    return s_hasErrors;
}

void FileParser::clearErrors() {
    s_hasErrors = false;
}

std::string FileParser::getExeDirectory() {
    wchar_t buffer[MAX_PATH];
    DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (len == 0) {
        return fs::current_path().string();
    }
    fs::path exePath(buffer);
    return exePath.parent_path().string();
}

std::optional<std::string> FileParser::findFile(const std::string& filename) {
    std::vector<fs::path> searchDirs = {
        fs::path(getExeDirectory()),
        fs::path(getExeDirectory()).parent_path(),
        fs::current_path()
    };
    
    for (const auto& dir : searchDirs) {
        fs::path fullPath = dir / filename;
        if (fs::exists(fullPath) && fs::is_regular_file(fullPath)) {
            return fullPath.string();
        }
    }
    
    return std::nullopt;
}

std::optional<std::string> FileParser::readFileContent(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return std::nullopt;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), 
                        std::istreambuf_iterator<char>());
    file.close();
    
    // Remove UTF-8 BOM if present
    if (content.size() >= 3 && 
        (unsigned char)content[0] == 0xEF && 
        (unsigned char)content[1] == 0xBB && 
        (unsigned char)content[2] == 0xBF) {
        content.erase(0, 3);
    }
    
    return content;
}

std::vector<std::string> FileParser::readFileLines(const std::string& filepath) {
    std::vector<std::string> lines;
    
    auto content = readFileContent(filepath);
    if (!content) {
        return lines;
    }
    
    std::istringstream stream(*content);
    std::string line;
    while (std::getline(stream, line)) {
        // Handle Windows line endings
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    
    return lines;
}

int FileParser::parseInt(const std::string& str, int defaultValue) {
    try {
        std::string trimmed = trim(str);
        if (trimmed.empty()) return defaultValue;
        return std::stoi(trimmed);
    } catch (...) {
        return defaultValue;
    }
}

char FileParser::parseChar(const std::string& str, char defaultValue) {
    std::string trimmed = trim(str);
    if (trimmed.empty()) return defaultValue;
    return trimmed[0];
}

std::string FileParser::trim(const std::string& str) {
    const char* whitespace = " \t\r\n";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

std::vector<std::string> FileParser::split(const std::string& str, char delimiter) {
    std::vector<std::string> parts;
    std::istringstream stream(str);
    std::string part;
    while (std::getline(stream, part, delimiter)) {
        parts.push_back(part);
    }
    return parts;
}

bool FileParser::startsWith(const std::string& str, const std::string& prefix) {
    if (str.size() < prefix.size()) return false;
    return str.compare(0, prefix.size(), prefix) == 0;
}
