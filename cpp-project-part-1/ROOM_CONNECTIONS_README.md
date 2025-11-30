# Room Connection System

## Overview
The game now uses a direction-based room transition system instead of numbers on the screen.

## How It Works

### Room Connections (RoomConnections.h)
Each room can have connections in four directions:
- **Left** - transition when player exits left edge
- **Right** - transition when player exits right edge  
- **Up** - transition when player exits top edge
- **Down** - transition when player exits bottom edge

### Configuration
Edit `RoomConnections.h` to define connections:

```cpp
// Example: Room 0 connects to Room 1 on the right
roomData.push_back({ 0, Direction::Right, 1 });

// Example: Room 1 connects back to Room 0 on the left
roomData.push_back({ 1, Direction::Left, 0 });
```

### Rules
1. **No corner transitions** - Players cannot transition from the four corners
2. **One transition per edge** - Each edge of a room can only lead to one other room
3. **Bidirectional connections** - If Room A connects to Room B, you should add the reverse connection manually

### Screen Files
- Numbers (0-9) in .screen files are now **ignored** for transitions
- They are still **displayed** for aesthetic purposes
- You can keep them in the design or remove them - it doesn't affect gameplay

### Example Configuration
```cpp
// Room 0 ? Room 1 (right)
roomData.push_back({ 0, Direction::Right, 1 });

// Room 1 ? Room 0 (left) and Room 2 (down)
roomData.push_back({ 1, Direction::Left, 0 });
roomData.push_back({ 1, Direction::Down, 2 });

// Room 2 ? Room 1 (up)
roomData.push_back({ 2, Direction::Up, 1 });
```

## Benefits
- **Unlimited rooms** - Not limited to digits 0-9
- **Clear connections** - Easy to see which rooms connect
- **Flexible design** - Can create complex room layouts
- **Better organization** - All connections in one place
