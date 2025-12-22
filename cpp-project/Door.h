#pragma once
#include "Player.h" 

class Door{
	char symbol;
	bool isOpen;
public:
	bool tryToOpen(Player& p);
};

