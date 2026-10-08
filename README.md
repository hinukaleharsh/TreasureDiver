# Treasure Diver

A small C++ treasure-hunting game built with [raylib](https://www.raylib.com/). Dive through increasingly dangerous ocean depths, collect treasure, and make it back to the boat before your oxygen or health runs out.

## Gameplay

- Explore a new, randomly generated underwater layout on every dive.
- Collect up to five kinds of treasure, from coins in the shallows to lost crowns in the abyss.
- Watch your oxygen and health as deeper water drains oxygen faster and slows your swimming.
- Avoid sharks, jellyfish, and mines; use harpoons to defend yourself from sharks.
- Return to the surface to sell your loot. Treasure is lost if you die before returning.
- Spend your earnings on upgrades, then work toward the $3,000 lifetime earnings goal.
- Your money, upgrades, lifetime earnings, and high score are saved in `save.txt`.

## Controls

| Key | Action |
| --- | --- |
| Arrow keys or `W` `A` `S` `D` | Swim |
| `Left Shift` | Boost (uses oxygen faster) |
| `Space` | Fire a harpoon |
| `1`–`5` | Buy the corresponding shop upgrade |
| `Enter` | Start a dive or continue from the results screen |
| `M` | Mute or unmute all sound |
| `R` | Reset the save after winning |

## Requirements

- A C++17 compiler
- [raylib](https://github.com/raysan5/raylib) headers and library

## Build

The project currently has no build-system files. With a MinGW-w64 compiler and raylib installed, run this from the repository root, adjusting the include and library paths for your installation:

```sh
g++ -std=c++17 src/main.cpp src/Dive.cpp src/Draw.cpp src/Globals.cpp src/Helpers.cpp src/SaveSystem.cpp src/Audio.cpp \
  -I/path/to/raylib/include -L/path/to/raylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm \
  -o TreasureDiver.exe
```

For raylib's bundled Windows `w64devkit`, the include and library paths are typically `C:/raylib/w64devkit/include` and `C:/raylib/w64devkit/lib`.

Run `TreasureDiver.exe` from the directory where you want `save.txt` to be stored. If no save file exists, the game starts with a fresh save.
