#include <iostream>
#include <windows.h>
#include "utils.h"

void gotoxy(int x, int y) {
    std::cout.flush();
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void hideCursor() {
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO curInfo;
    GetConsoleCursorInfo(hStdOut, &curInfo);
    curInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hStdOut, &curInfo);
}

void cls() {
    system("cls");
}

void setConsoleFont() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // On Windows Terminal the font is controlled by the host, skip font change
    bool isWindowsTerminal = (GetEnvironmentVariableW(L"WT_SESSION", nullptr, 0) > 0);

    if (!isWindowsTerminal) {
        CONSOLE_FONT_INFOEX cfi{};
        cfi.cbSize = sizeof(cfi);
        cfi.nFont = 0;
        cfi.dwFontSize.X = 0;   // Let the system pick width for the chosen height
        cfi.dwFontSize.Y = 18;  // Reasonable height for readability
        cfi.FontFamily = FF_DONTCARE;
        cfi.FontWeight = FW_NORMAL;
        
        // Prefer a TrueType font that supports Unicode box drawing
        wcscpy_s(cfi.FaceName, L"Consolas");
        if (!SetCurrentConsoleFontEx(hConsole, FALSE, &cfi)) {
            // Fallback
            wcscpy_s(cfi.FaceName, L"Lucida Console");
            SetCurrentConsoleFontEx(hConsole, FALSE, &cfi);
        }
    }
    
    // Set console screen buffer size to match 80x25
    COORD bufferSize;
    bufferSize.X = 80;
    bufferSize.Y = 25;
    SetConsoleScreenBufferSize(hConsole, bufferSize);
    
    // Set window size
    SMALL_RECT windowSize;
    windowSize.Left = 0;
    windowSize.Top = 0;
    windowSize.Right = 79;  // 80 columns (0-79)
    windowSize.Bottom = 24; // 25 rows (0-24)
    SetConsoleWindowInfo(hConsole, TRUE, &windowSize);
}