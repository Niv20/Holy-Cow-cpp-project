#include "Board.h"
#include "ScreenBuffer.h"
#include "DarkRoom.h"
#include "Glyph.h"
#include "utils.h"
#include <set>

// Factory method
Board* Board::create(bool silent) {
    if (silent) {
        return new SilentBoard();
    }
    return new DisplayBoard();
}

//                                 (__)
//'\-------------------------------(oo)
//  || DisplayBoard Implementation (__)
//  ||----------------------------||


void DisplayBoard::drawScreen(Screen& screen) {
    screen.draw();
}

void DisplayBoard::drawScreenWithDarkness(Screen& screen, const std::vector<Player>& players, int roomIdx) {
    DarkRoomManager::drawWithDarkness(screen, players, roomIdx);
}

void DisplayBoard::drawPlayers(const std::vector<Player>& players, int visibleRoomIdx) {
    constexpr wchar_t OVERLAP_ICON = L'O';
    ScreenBuffer& buffer = ScreenBuffer::getInstance();
    
    // Collect positions of players in current room
    std::vector<std::pair<Point, size_t>> playerPositions;
    for (size_t i = 0; i < players.size(); ++i) {
        if (players[i].getRoomIdx() == visibleRoomIdx) {
            playerPositions.push_back({players[i].getPosition(), i});
        }
    }
    
    // Draw players, handling overlapping positions
    std::set<std::pair<int,int>> drawnPositions;
    for (const auto& [pos, idx] : playerPositions) {
        auto posKey = std::make_pair(pos.getX(), pos.getY());
        if (drawnPositions.count(posKey) == 0) {
            // Check if another player is at the same position
            bool overlapping = false;
            for (const auto& [otherPos, otherIdx] : playerPositions) {
                if (otherIdx != idx && otherPos.getX() == pos.getX() && otherPos.getY() == pos.getY()) {
                    overlapping = true;
                    break;
                }
            }
            
            if (overlapping) {
                buffer.setChar(pos.getX(), pos.getY(), OVERLAP_ICON);
            } else {
                players[idx].draw();
            }
            drawnPositions.insert(posKey);
        }
    }
}

void DisplayBoard::drawLegend(Legend& legend, int roomIdx, int hearts, int points, char p1Inv, char p2Inv) {
    legend.drawLegend(roomIdx, hearts, points, p1Inv, p2Inv);
}

void DisplayBoard::refreshCell(Screen& screen, const Point& p) {
    screen.refreshCell(p);
}

void DisplayBoard::refreshCellWithDarkness(Screen& screen, const Point& p, const std::vector<Player>& players, int roomIdx) {
    DarkRoomManager::refreshCellWithDarkness(screen, p, players, roomIdx);
}

void DisplayBoard::updateDarknessAroundPlayers(Screen& screen, const std::vector<Player>& players,
                                                int roomIdx, const std::vector<Point>& prevPositions,
                                                const std::vector<Point>& torchSources) {
    DarkRoomManager::updateDarknessAroundPlayers(screen, players, roomIdx, prevPositions, torchSources);
}

void DisplayBoard::flush() {
    ScreenBuffer::getInstance().flush();
}

void DisplayBoard::clearScreen() {
    cls();
}

//                                (__)
//'\------------------------------(oo)
//  || SilentBoard Implementation (__)
//  ||---------------------------||

void SilentBoard::drawScreen(Screen& /*screen*/) {
    // Silent mode - no display
}

void SilentBoard::drawScreenWithDarkness(Screen& /*screen*/, const std::vector<Player>& /*players*/, int /*roomIdx*/) {
    // Silent mode - no display
}

void SilentBoard::drawPlayers(const std::vector<Player>& /*players*/, int /*visibleRoomIdx*/) {
    // Silent mode - no display
}

void SilentBoard::drawLegend(Legend& /*legend*/, int /*roomIdx*/, int /*hearts*/, int /*points*/, char /*p1Inv*/, char /*p2Inv*/) {
    // Silent mode - no display
}

void SilentBoard::refreshCell(Screen& /*screen*/, const Point& /*p*/) {
    // Silent mode - no display
}

void SilentBoard::refreshCellWithDarkness(Screen& /*screen*/, const Point& /*p*/, const std::vector<Player>& /*players*/, int /*roomIdx*/) {
    // Silent mode - no display
}

void SilentBoard::updateDarknessAroundPlayers(Screen& /*screen*/, const std::vector<Player>& /*players*/,
                                               int /*roomIdx*/, const std::vector<Point>& /*prevPositions*/,
                                               const std::vector<Point>& /*torchSources*/) {
    // Silent mode - no display
}

void SilentBoard::flush() {
    // Silent mode - no display
}

void SilentBoard::clearScreen() {
    // Silent mode - no display
}
