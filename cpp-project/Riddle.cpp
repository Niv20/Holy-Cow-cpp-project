#include "Riddle.h"
#include "RiddleData.h"
#include "Player.h"
#include "Game.h"
#include "Screen.h"
#include "ScreenBuffer.h"
#include "Menu.h"
#include "Glyph.h"
#include "GameRecorder.h"
#include "utils.h"
#include <conio.h>
#include <cstring>
#include <string>
#include <sstream>

using std::vector;
using std::string;
using std::map;

namespace {
    constexpr int MAX_QUESTION_DISPLAY_LENGTH = 45;
    constexpr char ANSWER_MIN_KEY = '1';
    constexpr char ANSWER_MAX_KEY = '4';
}

Riddle::Riddle() : correctAnswer('1'), points(100)
{
    question[0] = '\0';
    answer1[0] = '\0';
    answer2[0] = '\0';
    answer3[0] = '\0';
    answer4[0] = '\0';
}

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
    if (questionText.length() > MAX_QUESTION_DISPLAY_LENGTH) {
        questionText.resize(MAX_QUESTION_DISPLAY_LENGTH);
    }
    placeTextAt(QUESTION_ROW, QUESTION_COL, questionText, MAX_QUESTION_DISPLAY_LENGTH);

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
        RiddleKey key(rd.getRoomIdx(), rd.getPosition().getX(), rd.getPosition().getY());
        riddlesByPosition[key] = new Riddle(rd.getRiddle());
    }
}

void Riddle::handleEncounter(Player& player, 
                              map<RiddleKey, Riddle*>& riddlesByPosition,
                              Game& game) {
    
    int roomIdx = player.getRoomIdx();
    Point pos = player.getPosition();
    GameMode mode = game.getGameMode();
    bool isSilent = (mode == GameMode::LoadSilent);
    bool isLoadMode = (mode == GameMode::Load || mode == GameMode::LoadSilent);

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

    // Find player index for recording
    int playerIndex = 0;
    const auto& players = game.getPlayers();
    for (size_t i = 0; i < players.size(); ++i) {
        if (&players[i] == &player) {
            playerIndex = (int)i;
            break;
        }
    }

    // In load mode, get the answer from the recorder instead of waiting for input
    if (isLoadMode) {
        GameRecorder* recorder = game.getRecorder();
        if (recorder) {
            // Look for the next recorded answer for this cycle.
            // Accept either:
            // 1) KeyPress event with key '1'..'4'
            // 2) Typed ANSWER event from the steps file
            // Use shouldProcessEvent to check if the event is ready for this cycle
            char answer = '\0';
            int currentCycle = game.getGameCycle();
            
            if (recorder->shouldProcessEvent(currentCycle)) {
                const GameEvent& evt = recorder->peekNextEvent();
                if (evt.getType() == GameEventType::KeyPress) {
                    char key = evt.getKeyPressed();
                    if (key >= '1' && key <= '4') {
                        answer = key;
                        recorder->consumeNextEvent();
                    }
                }
                else if (evt.getType() == GameEventType::RiddleAnswer) {
                    // Stored as a string (usually "1".."4")
                    const std::string& a = evt.getRiddleAnswer();
                    if (!a.empty() && a[0] >= '1' && a[0] <= '4') {
                        answer = a[0];
                        recorder->consumeNextEvent();
                    }
                }
            }
            
            // Process the answer
            if (answer == '\0') {
                // No answer found for this cycle - move player back
                Point prevPos = pos;
                if (pos.getDiffX()) prevPos.setX(prevPos.getX() - pos.getDiffX());
                if (pos.getDiffY()) prevPos.setY(prevPos.getY() - pos.getDiffY());
                player.setPosition(prevPos);
                player.stop();
            } else {
                bool correct = (answer == riddle->getCorrectAnswer());
                
                // Record to results for verification
                if (mode == GameMode::LoadSilent) {
                    std::ostringstream oss;
                    oss << "Player " << (playerIndex + 1) << " answered riddle: " << answer 
                        << " (" << (correct ? "CORRECT" : "WRONG") << ")";
                    recorder->addActualResult(currentCycle, oss.str());
                }
                
                if (correct) {
                    game.addPoints(riddle->getPoints());
                    game.getScreen(roomIdx).setCharAt(pos, Glyph::Empty);

                    if (!isSilent) {
                        game.getScreen(roomIdx).refreshCell(pos);
                        ScreenBuffer::getInstance().flush();
                    }
                } else {
                    riddle->halvePoints();
                    game.reduceHearts(1);
                    Point prevPos = pos;
                    if (pos.getDiffX()) prevPos.setX(prevPos.getX() - pos.getDiffX());
                    if (pos.getDiffY()) prevPos.setY(prevPos.getY() - pos.getDiffY());
                    player.setPosition(prevPos);
                    player.stop();
                }
            }
        }
        return;  // Don't show UI in load mode
    }

    // Normal mode - show riddle UI and wait for input
    
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
    ScreenBuffer::getInstance().flush();

    // Wait for player answer
    char answer = '\0';
    while (true) {
        if (_kbhit()) {
            answer = _getch();
            
            // ESC - cancel riddle
            if (answer == ESC_KEY) { 
                Point prevPos = pos; 
                if (pos.getDiffX()) prevPos.setX(prevPos.getX() - pos.getDiffX()); 
                if (pos.getDiffY()) prevPos.setY(prevPos.getY() - pos.getDiffY()); 
                player.setPosition(prevPos); 
                player.stop(); 
                break; 
            }
            // Answer 1-4
            else if (answer >= ANSWER_MIN_KEY && answer <= ANSWER_MAX_KEY) {
                bool correct = (answer == riddle->getCorrectAnswer());
                
                // Record riddle event if in save mode
                GameRecorder* recorder = game.getRecorder();
                if (recorder && mode == GameMode::Save) {
                    recorder->recordRiddleEncounter(game.getGameCycle(), playerIndex, riddle->getQuestion());
                    recorder->recordRiddleAnswer(game.getGameCycle(), playerIndex, std::string(1, answer), correct);
                }
                
                if (correct) { 
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