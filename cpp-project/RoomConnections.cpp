#include "RoomConnections.h"
#include "Screen.h"
#include "ScreenMetadata.h"
#include <algorithm>

// Load connections from all screen metadata
void RoomConnections::loadFromScreens(const std::vector<Screen>& screens) {
    connections.clear();
    
    for (size_t roomIdx = 0; roomIdx < screens.size(); ++roomIdx) {
        const ScreenMetadata& meta = screens[roomIdx].getMetadata();
        
        // Load from connections map in metadata
        for (const auto& [dirStr, targetRoom] : meta.connections) {
            Direction dir = stringToDirection(dirStr);
            if (dir != Direction::None) {
                connections[(int)roomIdx][dir] = targetRoom;
            }
        }
    }
}
