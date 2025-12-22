#pragma once
#include <map>
#include <vector>
#include <string>

// Forward declaration
class Screen;
struct ScreenMetadata;

// Directions for room transitions
enum class Direction {
    None = -1,
    Left = 0,
    Right = 1,
    Up = 2,
    Down = 3
};

// Structure to hold room connections
struct RoomConnection {
    int fromRoom;
    Direction direction;
    int toRoom;
};

// Data structure to quickly lookup room transitions
class RoomConnections {
private:
    // Map: fromRoom -> Direction -> toRoom
    std::map<int, std::map<Direction, int>> connections;

    // Convert string direction to Direction enum
    static Direction stringToDirection(const std::string& dir) {
        if (dir == "LEFT") return Direction::Left;
        if (dir == "RIGHT") return Direction::Right;
        if (dir == "UP") return Direction::Up;
        if (dir == "DOWN") return Direction::Down;
        return Direction::None;
    }

public:
    // Default constructor - empty connections
    RoomConnections() = default;
    
    // Constructor from vector of connections (legacy support)
    RoomConnections(const std::vector<RoomConnection>& roomData) {
        for (const auto& conn : roomData) {
            connections[conn.fromRoom][conn.direction] = conn.toRoom;
        }
    }
    
    // Load connections from screen metadata
    void loadFromScreens(const std::vector<Screen>& screens);

    // Get the target room when moving in a direction from a room
    // Returns -1 if no connection exists
    int getTargetRoom(int fromRoom, Direction dir) const {
        auto roomIt = connections.find(fromRoom);
        if (roomIt == connections.end()) return -1;

        auto dirIt = roomIt->second.find(dir);
        if (dirIt == roomIt->second.end()) return -1;

        return dirIt->second;
    }

    bool hasConnection(int fromRoom, Direction dir) const {
        return getTargetRoom(fromRoom, dir) != -1;
    }
    
    // Add a single connection
    void addConnection(int fromRoom, Direction dir, int toRoom) {
        connections[fromRoom][dir] = toRoom;
    }
};
