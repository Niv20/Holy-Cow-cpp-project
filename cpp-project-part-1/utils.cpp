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
    
    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(cfi);
    cfi.nFont = 0;
    cfi.dwFontSize.X = 8;   // Width: 8 pixels
    cfi.dwFontSize.Y = 12;  // Height: 12 pixels
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    
    // Set font to "Terminal" (Raster font)
    wcscpy_s(cfi.FaceName, L"Terminal");
    
    SetCurrentConsoleFontEx(hConsole, FALSE, &cfi);
    
    // Also set console screen buffer size to match 80x25
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