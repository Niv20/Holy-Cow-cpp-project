#include "Door.h"
#include <cctype>

/*
The door symbol is uppercase, the key symbol is lowercase
The door can be opened if the player has the correct key
Door 'A' can be opened with key 'a', etc.
*/ 
bool Door::tryToOpen(Player& p) {

    if (p.getCarried() == std::tolower(static_cast<unsigned char>(symbol))) {
        isOpen = true;
        p.setCarried(' ');
        return true;
    }

    return false; // key doesn't match ORRRRRR player has no key
}