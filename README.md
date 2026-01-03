# Holy Cow - C++ Adventure Game

Welcome to **Holy Cow**, a console-based adventure game built with C++20. Navigate through a maze of rooms, solve riddles, avoid bombs, and help the cows find their way!

## ?? Introduction

Holy Cow is a text-based graphical adventure where players control characters to explore a multi-room world. The game features a custom engine that renders ASCII-based maps, handles physics (collisions, springs), and manages game state.

The goal is to navigate through various screens, solve puzzles, collect keys, and reach the final destination without losing all your hearts.

## ? Key Features

- **C++20 Codebase**: Utilizes modern C++ features for robust and efficient code.
- **Custom ASCII Engine**: Renders game worlds defined in text files.
- **Multi-Room World**: Seamless transitions between different screens/rooms.
- **Save/Load System**: Full game state serialization to save your progress.
- **Replay System**: Record your gameplay and replay it later (perfect for testing or sharing runs).
- **Silent Testing**: Automated verification mode to ensure game logic integrity.

## ??? Gameplay Mechanics

### Controls

| Player 1 | Action | Player 2 | Action |
|----------|--------|----------|--------|
| **W**    | Up     | **I**    | Up     |
| **S**    | Down   | **K**    | Down   |
| **A**    | Left   | **J**    | Left   |
| **D**    | Right  | **L**    | Right  |

- **ESC**: Pause Menu / Save Game
- **1-4**: Answer Riddles

### Game Elements

- **Players (@)**: You! Move around and interact with the world.
- **Walls (#)**: Solid obstacles you cannot pass through.
- **Keys (a-z)**: Collect these to open matching doors.
- **Doors (A-Z)**: Locked barriers. Require the corresponding lowercase key to open.
- **Special Doors (?)**: Advanced doors that may require multiple keys or specific switch configurations.
- **Springs (^)**: Step on these to get launched in a specific direction!
- **Switches (?)**: Toggle these to activate mechanisms or open special doors.
- **Pressure Buttons ($)**: Stand on these to temporarily remove specific walls.
- **Bombs (*)**: Dangerous explosives! They tick down and explode, destroying weak walls and hurting players.
- **Dark Zones**: Areas shrouded in darkness. You might need a light source (Torch `!`) to see.
- **Riddles**: Encounter question marks that challenge your knowledge. Answer correctly to gain points!

## ??? Technical Details

### File Formats

The game uses a custom file format for defining the world:

- **.screen files**: Visual representation of the level using ASCII characters.
- **Metadata**: Appended to the end of `.screen` files, defining logic like:
  - `CONNECT`: Room transitions (e.g., `CONNECT RIGHT 1`).
  - `DOOR`: Special door definitions with key/switch requirements.
  - `DARKZONES`: Areas that require a torch.
  - `PBUTTON`: Pressure button logic.

### Recording & Playback

The game includes a deterministic recording system:

- **adv-world.steps**: Records every input (cycle + keycode).
- **adv-world.result**: Records expected game events (transitions, riddles, game end) for verification.

### Command Line Arguments

You can run the game in different modes using command line flags:

1. **Normal Mode**: `cpp-project.exe`
   - Standard gameplay.

2. **Record Mode**: `cpp-project.exe -save`
   - Records your session to `.steps` and `.result` files.

3. **Playback Mode**: `cpp-project.exe -load`
   - Replays the recorded session visually.

4. **Silent Test Mode**: `cpp-project.exe -load -silent`
   - Runs the recording without graphics and verifies that the actual game events match the expected results.
   - Outputs `TEST PASSED` or `TEST FAILED`.

## ?? Build Instructions

### Prerequisites
- Visual Studio 2019 or newer (with C++ desktop development workload).
- Windows SDK.

### Steps
1. Open `cpp-project.sln` in Visual Studio.
2. Select the build configuration (Debug/Release) and platform (x64/x86).
3. Build the solution (`Ctrl+Shift+B`).
4. Run the game (`F5`).

## ?? Credits

Developed by Niv.
Based on a custom C++ game engine designed for educational and entertainment purposes.

---
*Moo-ve carefully and have fun!* ??
