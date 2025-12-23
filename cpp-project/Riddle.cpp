#include "Riddle.h"
#include "RiddleData.h"
#include "Player.h"
#include "Game.h"
#include "Screen.h"
#include "Menu.h"
#include "Glyph.h"
#include "utils.h"
#include <conio.h>
#include <cstring>
#include <string>

using std::vector;
using std::string;
using std::map;

Riddle::Riddle(const char q[], const char a1[], const char a2[], const char a3[], const char a4[], char correct)
{
    strcpy_s(question, MAX_QUESTION_LENGTH, q);
    strcpy_s(answer1, MAX_ANSWER_LENGTH, a1);
    strcpy_s(answer2, MAX_ANSWER_LENGTH, a2);
    strcpy_s(answer3, MAX_ANSWER_LENGTH, a3);
    strcpy_s(answer4, MAX_ANSWER_LENGTH, a4);
    correctAnswer = correct;
    points = 100;
}

// Written by AI!!!!!!!!!!
vector<string> Riddle::buildRiddleScreen(const vector<string>& templateScreen) const
{
    vector<string> riddleScreen = templateScreen;

    // Helper function to safely place text at specific position
    auto placeTextAt = [&](int row, int col, const string& text, int maxLength) {
        if (row < 0 || row >= (int)riddleScreen.size()) return;

        string& line = riddleScreen[row];

        // Place the text starting at the specified column
        for (int i = 0; i < (int)text.length() && i < maxLength && (col + i) < (int)line.size(); ++i) {
            line[col + i] = text[i];
        }
    };

    // Place question at its designated position
    string questionText(question);
    if (questionText.length() > 45) {
        questionText.resize(45);
    }
    placeTextAt(QUESTION_ROW, QUESTION_COL, questionText, 45);

    // Place answers at their designated positions
    placeTextAt(ANSWER1_ROW, ANSWER1_COL, string(answer1), MAX_ANSWER_LENGTH);
    // Swap visual positions of answers 2 and 3:
    placeTextAt(ANSWER3_ROW, ANSWER3_COL, string(answer2), MAX_ANSWER_LENGTH);
    placeTextAt(ANSWER2_ROW, ANSWER2_COL, string(answer3), MAX_ANSWER_LENGTH);
    placeTextAt(ANSWER4_ROW, ANSWER4_COL, string(answer4), MAX_ANSWER_LENGTH);

    return riddleScreen;
}

void Riddle::scanAllRiddles(map<RiddleKey, Riddle*>& riddlesByPosition) {
    vector<RiddleData> riddles = initRiddles();
    for (auto& rd : riddles) {
        RiddleKey key(rd.roomIdx, rd.position.getX(), rd.position.getY());
        riddlesByPosition[key] = new Riddle(rd.riddle);
    }
}

void Riddle::handleEncounter(Player& player, 
                              map<RiddleKey, Riddle*>& riddlesByPosition,
                              Game& game) {
    
    int roomIdx = player.getRoomIdx();
    Point pos = player.getPosition();

    // Find the riddle at this position
    RiddleKey exactKey(roomIdx, pos.getX(), pos.getY());
    Riddle* riddle = nullptr;

    if (riddlesByPosition.find(exactKey) != riddlesByPosition.end()) {
        riddle = riddlesByPosition[exactKey];
    } else {
        for (auto& pair : riddlesByPosition) {
            if (pair.first.getRoomIdx() == roomIdx) { 
                riddle = pair.second; 
                break; 
            }
        }
    }

    if (!riddle) return;

    // Get riddle template screen
    const vector<string>& templateScreen = Menu::getRiddleTemplate(); 
    if (templateScreen.empty()) {
        // No riddle template available - skip this riddle entirely
        // Remove the riddle glyph from the screen so player can pass
        game.getScreen(roomIdx).setCharAt(pos, Glyph::Empty);
        return;
    }

    // Build and display riddle screen
    vector<string> riddleScreenData = riddle->buildRiddleScreen(templateScreen); 
    Screen riddleScreen(riddleScreenData);
    
    cls(); 
    riddleScreen.draw();
    
    // Refresh legend (need to call through game)
    game.refreshLegendPublic();

    // Wait for player answer
    char answer = '\0';
    while (true) {
        if (_kbhit()) {
            answer = _getch();
            
            // ESC - cancel riddle
            if (answer == 27) { 
                Point prevPos = pos; 
                if (pos.getDiffX()) prevPos.setX(prevPos.getX() - pos.getDiffX()); 
                if (pos.getDiffY()) prevPos.setY(prevPos.getY() - pos.getDiffY()); 
                player.setPosition(prevPos); 
                player.stop(); 
                break; 
            }
            // Answer 1-4
            else if (answer >= '1' && answer <= '4') {
                if (answer == riddle->getCorrectAnswer()) { 
                    // Correct answer
                    game.addPoints(riddle->getPoints());
                    game.getScreen(roomIdx).setCharAt(pos, Glyph::Empty);
                }
                else { 
                    // Wrong answer
                    riddle->halvePoints(); 
                    game.reduceHearts(1);
                    Point prevPos = pos; 
                    if (pos.getDiffX()) prevPos.setX(prevPos.getX() - pos.getDiffX()); 
                    if (pos.getDiffY()) prevPos.setY(prevPos.getY() - pos.getDiffY()); 
                    player.setPosition(prevPos); 
                    player.stop(); 
                }
                break;
            }
        }
    }

    // Redraw game screen
    cls(); 
    game.drawEverythingPublic();
}