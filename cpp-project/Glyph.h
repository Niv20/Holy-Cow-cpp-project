#pragma once

// Glyph class - contains game character constants and classification methods
class Glyph {
public:
    // Basic glyphs (ASCII subset)
    static inline constexpr wchar_t Empty = L' ';
    static inline constexpr wchar_t First_Player = L'\x263A';   // ☺
    static inline constexpr wchar_t Second_Player = L'\x263B';  // ☻

    // Collectables
    static inline constexpr wchar_t Bomb = L'@';
    static inline constexpr wchar_t Torch = L'!';
    static inline constexpr wchar_t Obstacle = L'*';
    static inline constexpr wchar_t Spring = L'#'; 
    static inline constexpr wchar_t Riddle = L'?';
    static inline constexpr wchar_t Switch_Off = L'0'; 
    static inline constexpr wchar_t Switch_On = L'1'; 
    static inline constexpr wchar_t PressureSwitch = L'$'; 

    // Darkness shading characters (light to dark)
    static inline constexpr wchar_t Dark_Light = L'\x2591';  // ░
    static inline constexpr wchar_t Dark_Medium = L'\x2592'; // ▒
    static inline constexpr wchar_t Dark_Heavy = L'\x2593';  // ▓
    static inline constexpr wchar_t Dark_Full = L'\x2588';   // █

    // Bombable walls (weak walls)
    static inline constexpr wchar_t Bombable_Wall_H = L'-';
    static inline constexpr wchar_t Bombable_Wall_V = L'|';
    static inline constexpr wchar_t Bombable_Wall_Caret = L'^';

    // Unicode box drawing
    static inline constexpr wchar_t Wall_Single_TL = L'\x250C';
    static inline constexpr wchar_t Wall_Single_TR = L'\x2510';
    static inline constexpr wchar_t Wall_Single_BL = L'\x2514';
    static inline constexpr wchar_t Wall_Single_BR = L'\x2518';
    static inline constexpr wchar_t Wall_Single_H  = L'\x2500';
    static inline constexpr wchar_t Wall_Single_V  = L'\x2502';
    static inline constexpr wchar_t Wall_Single_TRgt = L'\x251C'; 
    static inline constexpr wchar_t Wall_Single_TLft = L'\x2524'; 
    static inline constexpr wchar_t Wall_Single_TDown = L'\x252C';
    static inline constexpr wchar_t Wall_Single_TUp   = L'\x2534';
    static inline constexpr wchar_t Wall_Single_Cross = L'\x253C';

    // Special door glyph
    static inline constexpr wchar_t SpecialDoor = L'\x25A0';

    // Classification methods
    static bool isDoor(wchar_t ch) { return ch >= L'A' && ch <= L'Z'; }
    static bool isKey(wchar_t ch) { return ch >= L'a' && ch <= L'z'; }
    static bool isRiddle(wchar_t ch) { return ch == Riddle || ch == L'\xFF1F'; }
    static bool isBomb(wchar_t ch) { return ch == Bomb; }
    static bool isTorch(wchar_t ch) { return ch == Torch; }
    static bool isBombableWall(wchar_t ch) { return ch == Bombable_Wall_Caret; }
    static bool isSpring(wchar_t ch) { return ch == Spring; }
    static bool isObstacle(wchar_t ch) { return ch == Obstacle; }
    static bool isSwitch(wchar_t ch) { return ch == Switch_Off || ch == Switch_On; }
    static bool isPressureButton(wchar_t ch) { return ch == PressureSwitch; }
    static bool isSpecialDoor(wchar_t ch) { return ch == SpecialDoor; }
    static bool isDarkness(wchar_t ch) { 
        return ch == Dark_Light || ch == Dark_Medium || ch == Dark_Heavy || ch == Dark_Full; 
    }

    static bool isWall(wchar_t ch) {
        if (ch == Wall_Single_TL || ch == Wall_Single_TR || ch == Wall_Single_BL || ch == Wall_Single_BR ||
            ch == Wall_Single_H || ch == Wall_Single_V || ch == Wall_Single_TRgt || ch == Wall_Single_TLft ||
            ch == Wall_Single_TDown || ch == Wall_Single_TUp || ch == Wall_Single_Cross ||
            ch == Bombable_Wall_H || ch == Bombable_Wall_V)
            return true;
        return false;
    }
};
