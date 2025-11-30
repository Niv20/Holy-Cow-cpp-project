#pragma once
#include <cwctype>

// Formerly Tiles.h renamed to Glyph.h; namespace renamed Tiles->Glyph
namespace Glyph {
    // Basic glyphs (ASCII subset)
    inline constexpr wchar_t Empty = L' ';
    inline constexpr wchar_t First_Player = L'\x00F6'; // ö
    inline constexpr wchar_t Second_Player = L'\x00FC'; // ü

    // Collectables
    inline constexpr wchar_t Bomb = L'@';
    inline constexpr wchar_t Torch = L'!';
    inline constexpr wchar_t Obstacle = L'*';
    inline constexpr wchar_t Riddle = L'?';

    // Bombable walls
    inline constexpr wchar_t Bombable_Wall_H = L'-';
    inline constexpr wchar_t Bombable_Wall_V = L'|';

    // Unicode box drawing
    inline constexpr wchar_t Wall_Single_TL = L'\x250C'; // ?
    inline constexpr wchar_t Wall_Single_TR = L'\x2510'; // ?
    inline constexpr wchar_t Wall_Single_BL = L'\x2514'; // ?
    inline constexpr wchar_t Wall_Single_BR = L'\x2518'; // ?
    inline constexpr wchar_t Wall_Single_H  = L'\x2500'; // ?
    inline constexpr wchar_t Wall_Single_V  = L'\x2502'; // ?
    inline constexpr wchar_t Wall_Single_TRgt = L'\x251C'; // ?
    inline constexpr wchar_t Wall_Single_TLft = L'\x2524'; // ?
    inline constexpr wchar_t Wall_Single_TDown = L'\x252C'; // ?
    inline constexpr wchar_t Wall_Single_TUp   = L'\x2534'; // ?
    inline constexpr wchar_t Wall_Single_Cross = L'\x253C'; // ?

    // Classification
    inline bool isDoor(wchar_t ch) { return ch >= L'A' && ch <= L'Z'; }
    inline bool isKey(wchar_t ch) { return ch >= L'a' && ch <= L'z'; }
    inline bool isRiddle(wchar_t ch) { return ch == Riddle || ch == L'\xFF1F'; } // include fullwidth variant
    inline bool isBomb(wchar_t ch) { return ch == Bomb; }
    inline bool isTorch(wchar_t ch) { return ch == Torch; }
    inline bool isBombableWall(wchar_t ch) { return ch == Bombable_Wall_H || ch == Bombable_Wall_V; }

    inline bool isWall(wchar_t ch) {
        if (ch == Wall_Single_TL || ch == Wall_Single_TR || ch == Wall_Single_BL || ch == Wall_Single_BR ||
            ch == Wall_Single_H || ch == Wall_Single_V || ch == Wall_Single_TRgt || ch == Wall_Single_TLft ||
            ch == Wall_Single_TDown || ch == Wall_Single_TUp || ch == Wall_Single_Cross ||
            ch == Bombable_Wall_H || ch == Bombable_Wall_V)
            return true;
        return false;
    }
}
