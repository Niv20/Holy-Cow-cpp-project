#include "Riddle.h"
#include <cstring>
#include <string>

using namespace std;

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
    placeTextAt(ANSWER2_ROW, ANSWER2_COL, string(answer2), MAX_ANSWER_LENGTH);
    placeTextAt(ANSWER3_ROW, ANSWER3_COL, string(answer3), MAX_ANSWER_LENGTH);
    placeTextAt(ANSWER4_ROW, ANSWER4_COL, string(answer4), MAX_ANSWER_LENGTH);

    return riddleScreen;
}