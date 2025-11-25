#include "Door.h"
#include <cctype>

bool Door::tryToOpen(Player& p) {

	if (p.getKeyIcon() == tolower(symbol)) {
		isOpen = true;
		p.setKeyIcon(' ');
		return true;
	}

	return false;
}