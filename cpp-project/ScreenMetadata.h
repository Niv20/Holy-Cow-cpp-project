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
class DarkZone {
private:
    Point topLeft_;      // Top-left corner
    Point bottomRight_;  // Bottom-right corner
    bool isLit_ = false; // Whether the zone has been lit
    
public:
    DarkZone() : topLeft_(0, 0), bottomRight_(0, 0) {}
    DarkZone(const Point& tl, const Point& br) : topLeft_(tl), bottomRight_(br) {}
    DarkZone(int x1, int y1, int x2, int y2) : topLeft_(x1, y1), bottomRight_(x2, y2) {}
    
    // Getters
    Point getTopLeft() const { return topLeft_; }
    Point getBottomRight() const { return bottomRight_; }
    bool getIsLit() const { return isLit_; }
    
    // Setters
    void setTopLeft(const Point& p) { topLeft_ = p; }
    void setBottomRight(const Point& p) { bottomRight_ = p; }
    void setIsLit(bool lit) { isLit_ = lit; }
    
    bool contains(const Point& p) const {
        return p.getX() >= topLeft_.getX() && p.getX() <= bottomRight_.getX() &&
               p.getY() >= topLeft_.getY() && p.getY() <= bottomRight_.getY();
    }
};

// Special door metadata from screen file
class DoorMetadata {
private:
    Point position_;
    std::vector<char> requiredKeys_;      // Required key letters (lowercase)
    std::vector<std::pair<Point, bool>> switchRequirements_; // {position, required_state}
    int targetRoom_ = -1;                 // Teleport target room (-1 = no teleport)
    Point targetPosition_;                // Teleport destination position
    
public:
    DoorMetadata() : position_(0, 0), targetRoom_(-1), targetPosition_(0, 0) {}
    
    // Getters
    Point getPosition() const { return position_; }
    const std::vector<char>& getRequiredKeys() const { return requiredKeys_; }
    const std::vector<std::pair<Point, bool>>& getSwitchRequirements() const { return switchRequirements_; }
    int getTargetRoom() const { return targetRoom_; }
    Point getTargetPosition() const { return targetPosition_; }
    
    // Setters
    void setPosition(const Point& p) { position_ = p; }
    void setTargetRoom(int room) { targetRoom_ = room; }
    void setTargetPosition(const Point& p) { targetPosition_ = p; }
    void addRequiredKey(char key) { requiredKeys_.push_back(key); }
    void addSwitchRequirement(const Point& pos, bool state) { switchRequirements_.push_back({pos, state}); }
};

// Pressure button metadata (for '$' tiles) - clears walls while held down
class PressureButtonMetadata {
private:
    Point position_;
    std::vector<Point> clearTargets_;
    
public:
    PressureButtonMetadata() : position_(0, 0) {}
    PressureButtonMetadata(const Point& pos) : position_(pos) {}
    
    // Getters
    Point getPosition() const { return position_; }
    const std::vector<Point>& getClearTargets() const { return clearTargets_; }
    
    // Setters
    void setPosition(const Point& p) { position_ = p; }
    void addClearTarget(const Point& p) { clearTargets_.push_back(p); }
    void setClearTargets(const std::vector<Point>& targets) { clearTargets_ = targets; }
};

// Room connection override
class ConnectionOverride {
private:
    std::string direction_;  // "LEFT", "RIGHT", "UP", "DOWN"
    int targetRoom_;
    
public:
    ConnectionOverride() : targetRoom_(-1) {}
    ConnectionOverride(const std::string& dir, int room) : direction_(dir), targetRoom_(room) {}
    
    // Getters
    const std::string& getDirection() const { return direction_; }
    int getTargetRoom() const { return targetRoom_; }
    
    // Setters
    void setDirection(const std::string& dir) { direction_ = dir; }
    void setTargetRoom(int room) { targetRoom_ = room; }
};

// Message box metadata - displays text in a box marked with 'T' in top-left corner
// The box has 3 lines of text that are centered within the box area
class MessageBoxMetadata {
private:
    Point anchorPos_;           // Position of 'T' marker (top-left of box interior)
    int boxWidth_ = 0;          // Width of the text area (detected from box)
    std::string line1_;         // Top line text
    std::string line2_;         // Middle line text  
    std::string line3_;         // Bottom line text
    bool hasMessage_ = false;   // Whether this screen has a message box
    
public:
    MessageBoxMetadata() : anchorPos_(0, 0) {}
    
    // Getters
    Point getAnchorPos() const { return anchorPos_; }
    int getBoxWidth() const { return boxWidth_; }
    const std::string& getLine1() const { return line1_; }
    const std::string& getLine2() const { return line2_; }
    const std::string& getLine3() const { return line3_; }
    bool getHasMessage() const { return hasMessage_; }
    
    // Setters
    void setAnchorPos(const Point& p) { anchorPos_ = p; }
    void setBoxWidth(int width) { boxWidth_ = width; }
    void setLine1(const std::string& line) { line1_ = line; }
    void setLine2(const std::string& line) { line2_ = line; }
    void setLine3(const std::string& line) { line3_ = line; }
    void setHasMessage(bool has) { hasMessage_ = has; }
};

// Complete metadata for a single screen
class ScreenMetadata {
private:
    std::vector<DoorMetadata> doors_;
    std::vector<PressureButtonMetadata> pressureButtons_;
    std::vector<DarkZone> darkZones_;
    std::vector<ConnectionOverride> connectionOverrides_;
    MessageBoxMetadata messageBox_;  // Optional message box for this screen
    
    // Room connections as a map: direction string -> target room index
    // Directions: "LEFT", "RIGHT", "UP", "DOWN"
    std::map<std::string, int> connections_;
    
public:
    // Getters
    const std::vector<DoorMetadata>& getDoors() const { return doors_; }
    std::vector<DoorMetadata>& getDoorsMutable() { return doors_; }
    const std::vector<PressureButtonMetadata>& getPressureButtons() const { return pressureButtons_; }
    std::vector<PressureButtonMetadata>& getPressureButtonsMutable() { return pressureButtons_; }
    const std::vector<DarkZone>& getDarkZones() const { return darkZones_; }
    std::vector<DarkZone>& getDarkZonesMutable() { return darkZones_; }
    const std::vector<ConnectionOverride>& getConnectionOverrides() const { return connectionOverrides_; }
    const MessageBoxMetadata& getMessageBox() const { return messageBox_; }
    MessageBoxMetadata& getMessageBoxMutable() { return messageBox_; }
    const std::map<std::string, int>& getConnections() const { return connections_; }
    std::map<std::string, int>& getConnectionsMutable() { return connections_; }
    
    // Modifiers
    void addDoor(const DoorMetadata& door) { doors_.push_back(door); }
    void addPressureButton(const PressureButtonMetadata& pb) { pressureButtons_.push_back(pb); }
    void addDarkZone(const DarkZone& zone) { darkZones_.push_back(zone); }
    void addConnectionOverride(const ConnectionOverride& co) { connectionOverrides_.push_back(co); }
    void setMessageBox(const MessageBoxMetadata& mb) { messageBox_ = mb; }
    void addConnection(const std::string& dir, int room) { connections_[dir] = room; }
    
    bool hasMetadata() const {
        return !doors_.empty() || !darkZones_.empty() || !pressureButtons_.empty() || 
               !connections_.empty() || messageBox_.getHasMessage();
    }
};
