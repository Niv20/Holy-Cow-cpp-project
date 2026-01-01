#pragma once
#include <vector>
#include <string>
#include "Screen.h"
#include "Player.h"
#include "Legend.h"

// Forward declarations
class Game;

// Abstract base class for game board rendering
// Implements polymorphism for display vs silent modes
class Board {
public:
    virtual ~Board() = default;
    
    // Pure virtual methods - must be implemented by derived classes
    virtual void drawScreen(Screen& screen) = 0;
    virtual void drawScreenWithDarkness(Screen& screen, const std::vector<Player>& players, int roomIdx) = 0;
    virtual void drawPlayers(const std::vector<Player>& players, int visibleRoomIdx) = 0;
    virtual void drawLegend(Legend& legend, int roomIdx, int hearts, int points, char p1Inv, char p2Inv) = 0;
    virtual void refreshCell(Screen& screen, const Point& p) = 0;
    virtual void refreshCellWithDarkness(Screen& screen, const Point& p, const std::vector<Player>& players, int roomIdx) = 0;
    virtual void updateDarknessAroundPlayers(Screen& screen, const std::vector<Player>& players, 
                                              int roomIdx, const std::vector<Point>& prevPositions,
                                              const std::vector<Point>& torchSources) = 0;
    virtual void flush() = 0;
    virtual void clearScreen() = 0;
    
    // Check if this board actually displays anything
    virtual bool isDisplayEnabled() const = 0;
    
    // Factory method to create appropriate board type
    static Board* create(bool silent);
};

// Display board - renders to screen (for normal play and load mode)
class DisplayBoard : public Board {
public:
    DisplayBoard() = default;
    ~DisplayBoard() override = default;
    
    void drawScreen(Screen& screen) override;
    void drawScreenWithDarkness(Screen& screen, const std::vector<Player>& players, int roomIdx) override;
    void drawPlayers(const std::vector<Player>& players, int visibleRoomIdx) override;
    void drawLegend(Legend& legend, int roomIdx, int hearts, int points, char p1Inv, char p2Inv) override;
    void refreshCell(Screen& screen, const Point& p) override;
    void refreshCellWithDarkness(Screen& screen, const Point& p, const std::vector<Player>& players, int roomIdx) override;
    void updateDarknessAroundPlayers(Screen& screen, const std::vector<Player>& players,
                                      int roomIdx, const std::vector<Point>& prevPositions,
                                      const std::vector<Point>& torchSources) override;
    void flush() override;
    void clearScreen() override;
    
    bool isDisplayEnabled() const override { return true; }
};

// Silent board - no rendering (for silent testing mode)
class SilentBoard : public Board {
public:
    SilentBoard() = default;
    ~SilentBoard() override = default;
    
    void drawScreen(Screen& screen) override;
    void drawScreenWithDarkness(Screen& screen, const std::vector<Player>& players, int roomIdx) override;
    void drawPlayers(const std::vector<Player>& players, int visibleRoomIdx) override;
    void drawLegend(Legend& legend, int roomIdx, int hearts, int points, char p1Inv, char p2Inv) override;
    void refreshCell(Screen& screen, const Point& p) override;
    void refreshCellWithDarkness(Screen& screen, const Point& p, const std::vector<Player>& players, int roomIdx) override;
    void updateDarknessAroundPlayers(Screen& screen, const std::vector<Player>& players,
                                      int roomIdx, const std::vector<Point>& prevPositions,
                                      const std::vector<Point>& torchSources) override;
    void flush() override;
    void clearScreen() override;
    
    bool isDisplayEnabled() const override { return false; }
};
