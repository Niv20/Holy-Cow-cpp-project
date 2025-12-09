#pragma once
// Embedded special door configuration text.
// Parsed by Game::loadSpecialDoors if external file is not found.
// Format:
//   D roomIdx x y       - Define door at position
//   K key1 key2 ...     - Required keys (lowercase letters)
//   S x y state         - Required switch state (0=off, 1=on)
//   T targetRoom x y    - Teleport destination (optional)
//   ---                 - End of door definition
constexpr const char* SPECIAL_DOORS_CONFIG = R"(D 4 31 18
K c o w
S 31 2 0
S 31 6 0
S 31 10 1
S 31 14 1
---			
D 0 2 2
K a
T 1 2 2
---
D 1 2 2
T 0 2 2
---)";

// Dear BODEK TARGILIM, we dont use the last two doors, because we dont think teleporting is good in our game, but you can add it if you want (by addding the door glyph ■ in the map...)