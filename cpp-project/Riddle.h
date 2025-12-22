#pragma once
#include <vector>
#include <string>
#include <map>

struct RiddleKey {
    int roomIdx;
    int x;
    int y;
    bool operator<(const RiddleKey& other) const {
        if (roomIdx != other.roomIdx) return roomIdx < other.roomIdx;
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
};

class Riddle {
	// TODO: Why static?
	static constexpr int MAX_QUESTION_LENGTH = 50;
	static constexpr int MAX_ANSWER_LENGTH = 16;

	// Positions in Riddle.screen for question and answers
	static constexpr int QUESTION_ROW = 4;
	static constexpr int QUESTION_COL = 12;
	
	static constexpr int ANSWER1_ROW = 9;
	static constexpr int ANSWER1_COL = 19;
	
	static constexpr int ANSWER2_ROW = 15;
	static constexpr int ANSWER2_COL = 19;
	
	static constexpr int ANSWER3_ROW = 9;
	static constexpr int ANSWER3_COL = 42;
	
	static constexpr int ANSWER4_ROW = 15;
	static constexpr int ANSWER4_COL = 42;

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
	
	// Static method to scan all riddles from RiddleData
	static void scanAllRiddles(std::map<RiddleKey, Riddle*>& riddlesByPosition);
	
	// Static method to handle riddle encounter with a player
	static void handleEncounter(class Player& player, 
	                             std::map<RiddleKey, Riddle*>& riddlesByPosition,
	                             class Game& game);
};