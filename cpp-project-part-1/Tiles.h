#pragma once
#include <cctype>

// This code was written by AI
namespace Tiles {

    inline constexpr char Empty = ' ';
    inline constexpr char First_Player = (char)148;  // ö
    inline constexpr char Second_Player = (char)129; // ü

    // Collectable Items
    inline constexpr char Bomb = '@';
    inline constexpr char Torch = '!';
	inline constexpr char Obstacle = '*';
	
    // Non-collectable tiles
    inline constexpr char Riddle = '?';
	// Door: 'A' to 'Z' (uppercase)
	// Key:  'a' to 'z' (lowercase)

	// Wall characters
    inline constexpr char Wall_Single_TL = (char)218;    // ┌
    inline constexpr char Wall_Single_TR = (char)191;    // ┐
    inline constexpr char Wall_Single_BL = (char)192;    // └
    inline constexpr char Wall_Single_BR = (char)217;    // ┘
    inline constexpr char Wall_Single_H  = (char)196;    // ─
    inline constexpr char Wall_Single_V  = (char)179;    // │
    inline constexpr char Wall_Single_TRgt = (char)195;  // ├
    inline constexpr char Wall_Single_TLft = (char)180;  // ┤
    inline constexpr char Wall_Single_TDown = (char)194; // ┬
    inline constexpr char Wall_Single_TUp   = (char)193; // ┴
    inline constexpr char Wall_Single_Cross = (char)197; // ┼

    // A distinct wall that can be destroyed by a bomb
    inline constexpr char Bombable_Wall_H = '-';
    inline constexpr char Bombable_Wall_V = '|';

    // Classification helpers
    
    // A..Z
    inline bool isDoor(char ch) {
        return std::isupper(static_cast<unsigned char>(ch)) != 0; 
    }

    // a..z
    inline bool isKey(char ch) {
        return std::islower(static_cast<unsigned char>(ch)) != 0;
    }

    // 0..9
    inline bool isRoomTransition(char ch) {
        return std::isdigit(static_cast<unsigned char>(ch)) != 0;
    }

    // ?
    inline bool isRiddle(char ch) {
        return ch == Riddle;
    }

	// @
    inline bool isBomb(char ch) {
        return ch == Bomb;
    }

	// !
    inline bool isTorch(char ch) {
        return ch == Torch;
    }

	// - or |
    inline bool isBombableWall(char ch) {
        return ch == Bombable_Wall_H || ch == Bombable_Wall_V;
    }

    // ┌ ┐ └ ┘ ─ │ ├ ┤ ┬ ┴ ┼ and bombable (-, |)
    inline bool isWall(char ch) {
        switch (static_cast<unsigned char>(ch)) {
            case static_cast<unsigned char>(Wall_Single_TL):
            case static_cast<unsigned char>(Wall_Single_TR):
            case static_cast<unsigned char>(Wall_Single_BL):
            case static_cast<unsigned char>(Wall_Single_BR):
            case static_cast<unsigned char>(Wall_Single_H):
            case static_cast<unsigned char>(Wall_Single_V):
            case static_cast<unsigned char>(Wall_Single_TRgt):
            case static_cast<unsigned char>(Wall_Single_TLft):
            case static_cast<unsigned char>(Wall_Single_TDown):
            case static_cast<unsigned char>(Wall_Single_TUp):
            case static_cast<unsigned char>(Wall_Single_Cross):
            case static_cast<unsigned char>(Bombable_Wall_H):
            case static_cast<unsigned char>(Bombable_Wall_V):
                return true;
            default:
                return false;
        }
    }
}