#pragma once
#include <vector>
#include <string>

class Riddle {
	// TODO: Why static?
	static constexpr int MAX_QUESTION_LENGTH = 50;
	static constexpr int MAX_ANSWER_LENGTH = 16;

	char question[MAX_QUESTION_LENGTH];
	char answer1[MAX_ANSWER_LENGTH];
	char answer2[MAX_ANSWER_LENGTH];
	char answer3[MAX_ANSWER_LENGTH];
	char answer4[MAX_ANSWER_LENGTH];

	char correctAnswer;
	int points;
public:
	Riddle(const char q[], const char a1[], const char a2[], const char a3[], const char a4[], char correct);
	
	// Display riddle on the graphical screen
	std::vector<std::string> buildRiddleScreen(const std::vector<std::string>& templateScreen) const;
	
	char getCorrectAnswer() const { return correctAnswer; }
	int getPoints() const { return points; }
	void halvePoints() { points /= 2; }
};