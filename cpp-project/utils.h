#pragma once

#include "GameRecorder.h"

// Parse command line arguments and determine game mode
GameMode parseCommandLineArgs(int argc, char* argv[]);

// Moves the console cursor to specific (x, y) coordinates
void gotoxy(int x, int y);

// Hides the blinking cursor for a better game look
void hideCursor();

// Clears the screen (system cls)
void cls();

// Sets console font to Raster for proper UTF-8 box-drawing display
void setConsoleFont();