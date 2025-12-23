#pragma once
#pragma execution_character_set("utf-8")
#include "Point.h"
#include <vector>
#include <string>
#include <map>

/*
                                                               (__)
'\-------------------------------------------------------------(oo)
  || For more info about metadata, read the METADATA.txt file  (__)
  ||----------------------------------------------------------||

  */

// Dark zone - rectangular area in the room
struct DarkZone {
    Point topLeft;      // Top-left corner
    Point bottomRight;  // Bottom-right corner
    bool isLit = false; // Whether the zone has been lit
    
    DarkZone() : topLeft(0, 0), bottomRight(0, 0) {}
    DarkZone(const Point& tl, const Point& br) : topLeft(tl), bottomRight(br) {}
    DarkZone(int x1, int y1, int x2, int y2) : topLeft(x1, y1), bottomRight(x2, y2) {}
    
    bool contains(const Point& p) const {
        return p.getX() >= topLeft.getX() && p.getX() <= bottomRight.getX() &&
               p.getY() >= topLeft.getY() && p.getY() <= bottomRight.getY();
    }
};

// Special door metadata from screen file
struct DoorMetadata {
    Point position;
    std::vector<char> requiredKeys;      // Required key letters (lowercase)
    std::vector<std::pair<Point, bool>> switchRequirements; // {position, required_state}
    int targetRoom = -1;                 // Teleport target room (-1 = no teleport)
    Point targetPosition;                // Teleport destination position
    
    DoorMetadata() : position(0, 0), targetRoom(-1), targetPosition(0, 0) {}
};

// Pressure button metadata (for '$' tiles) - clears walls while held down
struct PressureButtonMetadata {
    Point position;
    std::vector<Point> clearTargets;
};

// Room connection override
struct ConnectionOverride {
    std::string direction;  // "LEFT", "RIGHT", "UP", "DOWN"
    int targetRoom;
};

// Message box metadata - displays text in a box marked with 'T' in top-left corner
// The box has 3 lines of text that are centered within the box area
struct MessageBoxMetadata {
    Point anchorPos;           // Position of 'T' marker (top-left of box interior)
    int boxWidth = 0;          // Width of the text area (detected from box)
    std::string line1;         // Top line text
    std::string line2;         // Middle line text  
    std::string line3;         // Bottom line text
    bool hasMessage = false;   // Whether this screen has a message box
    
    MessageBoxMetadata() : anchorPos(0, 0) {}
};

// Complete metadata for a single screen
struct ScreenMetadata {
    std::vector<DoorMetadata> doors;
    std::vector<PressureButtonMetadata> pressureButtons;
    std::vector<DarkZone> darkZones;
    std::vector<ConnectionOverride> connectionOverrides;
    MessageBoxMetadata messageBox;  // Optional message box for this screen
    
    // Room connections as a map: direction string -> target room index
    // Directions: "LEFT", "RIGHT", "UP", "DOWN"
    std::map<std::string, int> connections;
    
    bool hasMetadata() const {
        return !doors.empty() || !darkZones.empty() || !pressureButtons.empty() || 
               !connections.empty() || messageBox.hasMessage;
    }
};
