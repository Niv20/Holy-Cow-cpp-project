#pragma once

class Key {
    char symbol;
public:
    Key(char s = ' ') : symbol(s) {}

    char get() const { return symbol; }
    bool valid() const { return symbol != ' '; }
    bool equals(const Key& other) const { return symbol == other.symbol; }
};