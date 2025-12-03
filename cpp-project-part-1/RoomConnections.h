#pragma once
#include <map>
#include <vector>

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

public:
    RoomConnections(const std::vector<RoomConnection>& roomData) {
        for (const auto& conn : roomData) {
            connections[conn.fromRoom][conn.direction] = conn.toRoom;
        }
    }

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
};

// Initialize all room connections
inline RoomConnections initRoomConnections() {

    // Map: fromRoom -> Direction -> toRoom
    std::vector<RoomConnection> roomData;

    // Room 0 connections
    roomData.push_back({ 0, Direction::Right, 1 });
    roomData.push_back({ 0, Direction::Left, 6 });

    // Room 1 connections
    roomData.push_back({ 1, Direction::Right, 2 });
    roomData.push_back({ 1, Direction::Left, 0 });

    // Room 2 connections
    roomData.push_back({ 2, Direction::Up, 3 });
    roomData.push_back({ 2, Direction::Down, 4 });
    roomData.push_back({ 2, Direction::Right, 5 });
    roomData.push_back({ 2, Direction::Left, 1 });

	// Room 3 connections
    roomData.push_back({ 3, Direction::Down, 2 });

	// Room 4 connections
    roomData.push_back({ 4, Direction::Up, 2 });

	// Room 5 connections
    roomData.push_back({ 5, Direction::Left, 2 });
    roomData.push_back({ 5, Direction::Right, 2 }); // optional if 2->5 only; keep symmetry if needed

    return RoomConnections(roomData);
}
