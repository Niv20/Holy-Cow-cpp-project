#include "ScreenBuffer.h"

ScreenBuffer& ScreenBuffer::getInstance() {
    static ScreenBuffer instance;
    return instance;
}

ScreenBuffer::ScreenBuffer() {
    buffer_.resize(HEIGHT, std::vector<wchar_t>(WIDTH, L' '));
    previousBuffer_.resize(HEIGHT, std::vector<wchar_t>(WIDTH, L'\0')); // Different from buffer to force first flush
    hConsole_ = GetStdHandle(STD_OUTPUT_HANDLE);
}

void ScreenBuffer::clear() {
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            buffer_[y][x] = L' ';
        }
    }
    dirty_ = true;
}

void ScreenBuffer::setChar(int x, int y, wchar_t ch) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
    if (buffer_[y][x] != ch) {
        buffer_[y][x] = ch;
        dirty_ = true;
    }
}

wchar_t ScreenBuffer::getChar(int x, int y) const {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return L' ';
    return buffer_[y][x];
}

void ScreenBuffer::flush() {
    if (!dirty_) return;

    // Write entire buffer to console, line by line
    // This is more efficient than character-by-character and eliminates flicker
    for (int y = 0; y < HEIGHT; ++y) {
        // Check if this line changed
        bool lineChanged = false;
        for (int x = 0; x < WIDTH; ++x) {
            if (buffer_[y][x] != previousBuffer_[y][x]) {
                lineChanged = true;
                break;
            }
        }
        
        if (lineChanged) {
            // Build the line string
            std::wstring line;
            line.reserve(WIDTH);
            for (int x = 0; x < WIDTH; ++x) {
                line.push_back(buffer_[y][x]);
                previousBuffer_[y][x] = buffer_[y][x];
            }
            
            // Write the entire line at once
            COORD pos{0, (SHORT)y};
            SetConsoleCursorPosition(hConsole_, pos);
            DWORD written;
            WriteConsoleW(hConsole_, line.c_str(), (DWORD)line.size(), &written, nullptr);
        }
    }
    
    dirty_ = false;
}

void ScreenBuffer::invalidate() {
    // Reset previousBuffer to force full redraw on next flush
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            previousBuffer_[y][x] = L'\0';
        }
    }
    dirty_ = true;
}
