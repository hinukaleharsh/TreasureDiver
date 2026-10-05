# Treasure Diver

A small C++ treasure-hunting game built with [raylib](https://www.raylib.com/). Dive through increasingly dangerous ocean depths, collect treasure, and make it back to the boat before your oxygen or health runs out.

## Gameplay

- Explore a new, randomly generated underwater layout on every dive.
- Collect up to five kinds of treasure, from coins in the shallows to lost crowns in the abyss.
- Watch your oxygen and health as deeper water drains oxygen faster and slows your swimming.
- Avoid sharks, jellyfish, and mines; use harpoons to defend yourself from sharks.
- Return to the surface to sell your loot. Treasure is lost if you die before returning.
- Spend your earnings on upgrades, then work toward the $3,000 lifetime earnings goal.
- Your money, upgrades, lifetime earnings, and high score are saved between sessions. The desktop build uses `save.txt`; the browser build stores progress in browser storage.

## Controls

| Key | Action |
| --- | --- |
| Arrow keys or `W` `A` `S` `D` | Swim |
| `Left Shift` | Boost (uses oxygen faster) |
| `Space` | Fire a harpoon |
| `1`–`5` | Buy the corresponding shop upgrade |
| `Enter` | Start a dive or continue from the results screen |
| `R` | Reset the save after winning |

## Requirements

- A C++17 compiler
- [raylib](https://github.com/raysan5/raylib) headers and library

## Build

### Desktop

With CMake installed, configure and build from the repository root:

```sh
cmake -S . -B build
cmake --build build
```

Alternatively, with a MinGW-w64 compiler and raylib installed, adjust the include and library paths for your installation:

```sh
g++ -std=c++17 src/main.cpp src/Dive.cpp src/Draw.cpp src/Globals.cpp src/Helpers.cpp src/SaveSystem.cpp \
  -I/path/to/raylib/include -L/path/to/raylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm \
  -o TreasureDiver.exe
```

For raylib's bundled Windows `w64devkit`, the include and library paths are typically `C:/raylib/w64devkit/include` and `C:/raylib/w64devkit/lib`.

Run the game from the directory where you want `save.txt` to be stored. If no save file exists, it starts with a fresh save.

### WebAssembly

Play the C++ game in a browser at:

<https://hinukaleharsh.github.io/Treasure-Diver/>

To publish the browser game, set the repository's **Settings → Pages → Build and deployment → Source** to **GitHub Actions**. The GitHub Actions workflow then builds the C++ game to WebAssembly and deploys it whenever game or build files change on `main`.

Click the game to focus it before using the keyboard controls. Browser progress is stored locally in that browser and is separate from the desktop `save.txt` file.
