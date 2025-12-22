#include "RiddleData.h"
#include "FileParser.h"
#include <sstream>

using std::vector;
using std::string;

// Parse riddles from riddles.txt file
// Format:
//   RIDDLE <room_index> <x> <y>
//   QUESTION <question text>
//   ANSWER1 <answer text>
//   ANSWER2 <answer text>
//   ANSWER3 <answer text>
//   ANSWER4 <answer text>
//   CORRECT <1-4>
//   ---
vector<RiddleData> RiddleData::loadFromFile() {
    vector<RiddleData> riddles;
    
    // Find riddles.txt file
    auto filepath = FileParser::findFile("riddles.txt");
    if (!filepath) {
        FileParser::reportError("riddles.txt not found in any search directory");
        return riddles;
    }
    
    // Read file lines
    vector<string> lines = FileParser::readFileLines(*filepath);
    if (lines.empty()) {
        FileParser::reportError("riddles.txt is empty or could not be read");
        return riddles;
    }
    
    // Parsing state
    bool inRiddle = false;
    int roomIdx = 0;
    int posX = 0, posY = 0;
    string question;
    string answer1, answer2, answer3, answer4;
    char correct = '1';
    int lineNum = 0;
    
    auto resetState = [&]() {
        inRiddle = false;
        question.clear();
        answer1.clear();
        answer2.clear();
        answer3.clear();
        answer4.clear();
        correct = '1';
    };
    
    auto validateAndAddRiddle = [&]() -> bool {
        // Validate all fields are present
        if (question.empty()) {
            FileParser::reportError("Riddle at line " + std::to_string(lineNum) + ": missing QUESTION");
            return false;
        }
        if (answer1.empty() || answer2.empty() || answer3.empty() || answer4.empty()) {
            FileParser::reportError("Riddle at line " + std::to_string(lineNum) + ": missing one or more ANSWER fields");
            return false;
        }
        if (correct < '1' || correct > '4') {
            FileParser::reportError("Riddle at line " + std::to_string(lineNum) + ": CORRECT must be 1-4");
            return false;
        }
        
        // Create riddle
        riddles.push_back({
            roomIdx,
            Point(posX, posY),
            Riddle(question.c_str(), answer1.c_str(), answer2.c_str(), 
                   answer3.c_str(), answer4.c_str(), correct)
        });
        return true;
    };
    
    for (const auto& line : lines) {
        lineNum++;
        string trimmed = FileParser::trim(line);
        
        // Skip empty lines and comments
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }
        
        // End of riddle marker
        if (FileParser::startsWith(trimmed, "---")) {
            if (inRiddle) {
                validateAndAddRiddle();
                resetState();
            }
            continue;
        }
        
        // Parse commands
        std::istringstream iss(trimmed);
        string cmd;
        iss >> cmd;
        
        if (cmd == "RIDDLE") {
            // Start new riddle: RIDDLE <room> <x> <y>
            if (inRiddle) {
                // Previous riddle wasn't closed properly
                FileParser::reportError("Line " + std::to_string(lineNum) + ": new RIDDLE started before previous one was closed with ---");
                validateAndAddRiddle();
                resetState();
            }
            
            inRiddle = true;
            iss >> roomIdx >> posX >> posY;
        }
        else if (cmd == "QUESTION" && inRiddle) {
            // Get rest of line as question
            std::getline(iss, question);
            question = FileParser::trim(question);
        }
        else if (cmd == "ANSWER1" && inRiddle) {
            std::getline(iss, answer1);
            answer1 = FileParser::trim(answer1);
        }
        else if (cmd == "ANSWER2" && inRiddle) {
            std::getline(iss, answer2);
            answer2 = FileParser::trim(answer2);
        }
        else if (cmd == "ANSWER3" && inRiddle) {
            std::getline(iss, answer3);
            answer3 = FileParser::trim(answer3);
        }
        else if (cmd == "ANSWER4" && inRiddle) {
            std::getline(iss, answer4);
            answer4 = FileParser::trim(answer4);
        }
        else if (cmd == "CORRECT" && inRiddle) {
            string correctStr;
            iss >> correctStr;
            correct = FileParser::parseChar(correctStr, '1');
        }
        else if (inRiddle) {
            // Unknown command inside riddle - warn but continue
            FileParser::reportError("Line " + std::to_string(lineNum) + ": unknown command '" + cmd + "' inside riddle definition");
        }
    }
    
    // Handle unclosed riddle at end of file
    if (inRiddle) {
        FileParser::reportError("End of file reached with unclosed riddle (missing ---)");
        validateAndAddRiddle();
    }
    
    return riddles;
}
