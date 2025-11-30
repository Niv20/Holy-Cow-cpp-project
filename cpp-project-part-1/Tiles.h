#pragma once
#include <cwctype>

// This file was written by AI
namespace Tiles {
    // Basic tiles (ASCII subset)
    inline constexpr wchar_t Empty = L' ';
    inline constexpr wchar_t First_Player = L'ö';
    inline constexpr wchar_t Second_Player = L'ü';

    // Collectables
    inline constexpr wchar_t Bomb = L'@';
    inline constexpr wchar_t Torch = L'!';
    inline constexpr wchar_t Obstacle = L'*';
    inline constexpr wchar_t Riddle = L'?';

    // Bombable walls
    inline constexpr wchar_t Bombable_Wall_H = L'-';
    inline constexpr wchar_t Bombable_Wall_V = L'|';

    // Unicode box drawing (match file content ┌─┐ etc.)
    inline constexpr wchar_t Wall_Single_TL = L'┌';
    inline constexpr wchar_t Wall_Single_TR = L'┐';
    inline constexpr wchar_t Wall_Single_BL = L'└';
    inline constexpr wchar_t Wall_Single_BR = L'┘';
    inline constexpr wchar_t Wall_Single_H  = L'─';
    inline constexpr wchar_t Wall_Single_V  = L'│';
    inline constexpr wchar_t Wall_Single_TRgt = L'├';
    inline constexpr wchar_t Wall_Single_TLft = L'┤';
    inline constexpr wchar_t Wall_Single_TDown = L'┬';
    inline constexpr wchar_t Wall_Single_TUp   = L'┴';
    inline constexpr wchar_t Wall_Single_Cross = L'┼';

    // Classification (wchar_t)
    inline bool isDoor(wchar_t ch) { return ch >= L'A' && ch <= L'Z'; }
    inline bool isKey(wchar_t ch) { return ch >= L'a' && ch <= L'z'; }
    inline bool isRoomTransition(wchar_t ch) { return ch >= L'0' && ch <= L'9'; }
    inline bool isRiddle(wchar_t ch) { return ch == Riddle; }
    inline bool isBomb(wchar_t ch) { return ch == Bomb; }
    inline bool isTorch(wchar_t ch) { return ch == Torch; }
    inline bool isBombableWall(wchar_t ch) { return ch == Bombable_Wall_H || ch == Bombable_Wall_V; }

    inline bool isWall(wchar_t ch) {
        switch (ch) {
            case Wall_Single_TL: case Wall_Single_TR: case Wall_Single_BL: case Wall_Single_BR:
            case Wall_Single_H: case Wall_Single_V: case Wall_Single_TRgt: case Wall_Single_TLft:
            case Wall_Single_TDown: case Wall_Single_TUp: case Wall_Single_Cross:
            case Bombable_Wall_H: case Bombable_Wall_V:
                return true;
            default: return false;
        }
    }
}