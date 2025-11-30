#pragma once
#pragma execution_character_set("utf-8")
#include <vector>
#include <string>
#include "Riddle.h"
#include "Point.h"

using std::vector;
using std::string;

// Structure to hold riddle data with room and position association
struct RiddleData {
    int roomIdx;
    Point position;  // Specific coordinates where this riddle appears
    Riddle riddle;
};

// Initialize all riddles with their associated rooms and positions
inline vector<RiddleData> initRiddles() {
    vector<RiddleData> riddles;
    
    // Riddle for Room 0 at a specific position
    riddles.push_back({
        0,  // Room index
        Point(40, 12),  // Position in the room (x, y)
        Riddle(
            "What do cows use to do math?",
            "computer",
            "cow-culator",
            "their fingers",
            "abacus",
            '2'  // cow-culator is correct
        )
    });
    
    // Riddle for Room 1 at a specific position
    riddles.push_back({
        1,  // Room index
        Point(45, 10),  // Position in the room (x, y)
        Riddle(
            "What is a cow's favorite type of movie?",
            "action",
            "horror",
            "moo-sicals",
            "comedy",
            '3'  // moo-sicals is correct
        )
    });
    
    return riddles;
}