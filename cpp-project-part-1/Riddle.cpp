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

    // Place question at 'q'
    for (auto& line : riddleScreen) {
        size_t qPos = line.find('q');
        if (qPos != string::npos) {
            string questionText(question);
            if (questionText.length() > 45) questionText.resize(45);
            for (size_t i = 0; i < questionText.length() && qPos + i < line.size(); ++i) line[qPos + i] = questionText[i];
            break;
        }
    }

    // Helper: find placeholder character in any line, replace starting exactly at that position
    auto replaceAtPlaceholder = [&](char placeholder, const string& text) {
        for (size_t lineIdx = 0; lineIdx < riddleScreen.size(); ++lineIdx) {
            auto& line = riddleScreen[lineIdx];
            size_t pos = line.find(placeholder);
            if (pos != string::npos) {
                // Check if this placeholder follows the pattern "||__|| " (answer box)
                if (pos >= 7 && line.substr(pos - 7, 7) == "||__|| ") {
                    // Replace starting exactly at placeholder position
                    size_t i = 0;
                    for (; i < text.size() && pos + i < line.size(); ++i) {
                        line[pos + i] = text[i];
                    }
                    // Fill remaining space with spaces until boundary
                    size_t end = line.find('|', pos + i);
                    if (end == string::npos) end = line.size();
                    for (; pos + i < end; ++i) line[pos + i] = ' ';
                    return; // done
                }
            }
        }
    };

    // Template has: line 9: ||__|| A ... ||__|| C
    //               line 15: ||__|| B ... ||__|| D
    // Replace A with answer1, B with answer2, C with answer3, D with answer4
    replaceAtPlaceholder('A', string(answer1));
    replaceAtPlaceholder('B', string(answer2));
    replaceAtPlaceholder('C', string(answer3));
    replaceAtPlaceholder('D', string(answer4));

    return riddleScreen;
}