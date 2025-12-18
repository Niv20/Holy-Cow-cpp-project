#pragma once
// Fallback special door configuration (used if metadata not found in screen files)
// The preferred way is to include door data in each .screen file's metadata section.
//
// Screen file metadata format (at end of .screen file):
//   === METADATA ===
//   DOOR <x> <y>
//   KEYS <key1> <key2> ...     # Required keys (lowercase letters)
//   SWITCH <x> <y> <state>     # Required switch state (0=off, 1=on)
//   TARGET <room> <x> <y>      # Teleport destination (optional)
//   ---
//
// Legacy embedded format (parsed if no metadata in screen file):
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

// Note: The last two doors are optional teleport doors - add the door glyph (■) in the map to enable them