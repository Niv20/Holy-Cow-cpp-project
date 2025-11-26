#include "Riddle.h"
#include <cstring>

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

// code with AI!
vector<string> Riddle::buildRiddleScreen(const vector<string>& templateScreen) const
{
	vector<string> riddleScreen = templateScreen;
	
	// Insert question at line 4 (overwrite starting at 'q' without changing length)
	string& questionLine = riddleScreen[4];
	size_t qPos = questionLine.find('q');
	if (qPos != string::npos) {
		string questionText(question);
		if (questionText.length() > 45) questionText = questionText.substr(0, 45);
		for (size_t i = 0; i < questionText.length() && qPos + i < questionLine.size(); ++i) {
			questionLine[qPos + i] = questionText[i];
		}
	}
	
	// Answer line 1 (line 8) placeholders A and B
	string& answerLine1 = riddleScreen[8];
	size_t aPos = answerLine1.find('A');
	if (aPos != string::npos) {
		string a1Str(answer1);
		if (a1Str.length() > 12) a1Str = a1Str.substr(0, 12);
		for (size_t i = 0; i < a1Str.length() && aPos + i < answerLine1.size(); ++i) {
			answerLine1[aPos + i] = a1Str[i];
		}
	}
	
	size_t bPos = answerLine1.find('B');
	if (bPos != string::npos) {
		string a2Str(answer2);
		if (a2Str.length() > 12) a2Str = a2Str.substr(0, 12);
		for (size_t i = 0; i < a2Str.length() && bPos + i < answerLine1.size(); ++i) {
			answerLine1[bPos + i] = a2Str[i];
		}
	}
	
	// Answer line 2 (line 13) placeholders C and D
	string& answerLine2 = riddleScreen[13];
	size_t cPos = answerLine2.find('C');
	if (cPos != string::npos) {
		string a3Str(answer3);
		if (a3Str.length() > 12) a3Str = a3Str.substr(0, 12);
		for (size_t i = 0; i < a3Str.length() && cPos + i < answerLine2.size(); ++i) {
			answerLine2[cPos + i] = a3Str[i];
		}
	}
	
	size_t dPos = answerLine2.find('D');
	if (dPos != string::npos) {
		string a4Str(answer4);
		if (a4Str.length() > 12) a4Str = a4Str.substr(0, 12);
		for (size_t i = 0; i < a4Str.length() && dPos + i < answerLine2.size(); ++i) {
			answerLine2[dPos + i] = a4Str[i];
		}
	}
	
	return riddleScreen;
}