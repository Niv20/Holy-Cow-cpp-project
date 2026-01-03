#pragma once
#pragma execution_character_set("utf-8")
#include <vector>
#include <string>
#include "Riddle.h"
#include "Point.h"

using std::vector;
using std::string;

// Structure to hold riddle data with room and position association
class RiddleData {
private:
    int roomIdx_;
    Point position_;  // Specific coordinates where this riddle appears
    Riddle riddle_;
    
public:
    RiddleData() : roomIdx_(-1), position_(0, 0) {}
    RiddleData(int room, const Point& pos, const Riddle& r) 
        : roomIdx_(room), position_(pos), riddle_(r) {}
    
    // Getters
    int getRoomIdx() const { return roomIdx_; }
    Point getPosition() const { return position_; }
    const Riddle& getRiddle() const { return riddle_; }
    Riddle& getRiddleMutable() { return riddle_; }
    
    // Setters
    void setRoomIdx(int room) { roomIdx_ = room; }
    void setPosition(const Point& pos) { position_ = pos; }
    void setRiddle(const Riddle& r) { riddle_ = r; }
    
    // Load riddles from riddles.txt file
    // Returns empty vector if file not found or has critical errors
    // Individual parsing errors are reported but don't stop loading
    static vector<RiddleData> loadFromFile();
};

// Initialize all riddles from file
inline vector<RiddleData> initRiddles() {
    return RiddleData::loadFromFile();
}