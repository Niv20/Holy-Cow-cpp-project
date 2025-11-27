#pragma once
#pragma execution_character_set("utf-8")
#include <vector>
#include <string>
#include "Riddle.h"

using std::vector;
using std::string;

// Structure to hold riddle data with room association
struct RiddleData {
    int roomIdx;
    Riddle riddle;
};

// Initialize all riddles with their associated rooms
inline vector<RiddleData> initRiddles() {
    vector<RiddleData> riddles;
    
    // Riddle for Room 0
    riddles.push_back({
        0,  // Room index
        Riddle(
            "What do cows use to do math?",
            "computer",
            "cow-culator",
            "their fingers",
            "abacus",
            '2'  // cow-culator is correct
        )
    });
    
    // Riddle for Room 1
    riddles.push_back({
        1,  // Room index
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