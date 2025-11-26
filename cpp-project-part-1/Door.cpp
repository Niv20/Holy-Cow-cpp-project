#include "Door.h"
#include <cctype>

/*
The door symbol is uppercase, the key symbol is lowercase
The door can be opened if the player has the correct key
Door 'A' can be opened with key 'a', etc.
*/ 
bool Door::tryToOpen(Player& p) {

	if (p.getKeyIcon() == tolower(symbol)) {
		isOpen = true;
		p.setKeyIcon(' ');
		return true;
	}

	return false; // key doesn't match OR player has no key
}