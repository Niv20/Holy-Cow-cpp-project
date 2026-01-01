#pragma once
#include <vector>
#include <string>
#include <windows.h>

// Double buffering for flicker-free console rendering.
// All draw operations write to this buffer, then flush() writes everything at once.
class ScreenBuffer {
public:
    static constexpr int WIDTH = 80;
    static constexpr int HEIGHT = 25;

    // Get the singleton instance
    static ScreenBuffer& getInstance();

    // Clear the buffer (fill with spaces)
    void clear();

    // Set a character at a position
    void setChar(int x, int y, wchar_t ch);

    // Get a character at a position
    wchar_t getChar(int x, int y) const;

    // Write entire buffer to console (call once per frame)
    void flush();

    // Mark that buffer content has changed and needs flushing
    void markDirty() { dirty_ = true; }
    
    // Check if buffer is dirty
    bool isDirty() const { return dirty_; }

private:
    ScreenBuffer();
    ~ScreenBuffer() = default;
    
    // Disable copying
    ScreenBuffer(const ScreenBuffer&) = delete;
    ScreenBuffer& operator=(const ScreenBuffer&) = delete;

    std::vector<std::vector<wchar_t>> buffer_;
    std::vector<std::vector<wchar_t>> previousBuffer_; // For dirty-region optimization
    bool dirty_ = true;
    HANDLE hConsole_;
};
