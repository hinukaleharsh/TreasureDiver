#pragma once
#include <vector>
#include <string>
#include "raylib.h"
#include "Types.h"

// -------------------------------------------------------------
// GLOBALS  -  the game's "memory". Every file that needs to read
// or change the current game state includes this header.
// Actual storage lives in Globals.cpp ("extern" means: "this
// variable exists somewhere else, just trust me").
// -------------------------------------------------------------

extern SaveData save;
extern State    state;

extern Diver    diver;
extern Camera2D cam;
extern std::vector<Treasure> treasures;
extern std::vector<Shark>    sharks;
extern std::vector<Jelly>    jellies;
extern std::vector<Mine>     mines;
extern std::vector<Current>  currents;
extern std::vector<Harpoon>  harpoons;

extern int   bag[5];             // inventory: how many of each treasure type
extern float maxDepthReached;
extern int   harpoonsLeft;
extern float maxOxygen;

extern std::string message;      // short pop-up text during a dive
extern float messageTimer;
extern float flashTimer;         // red screen flash when hurt
extern std::string shopMsg;
extern bool  hasWon;
extern int   settingsSel;        // audio settings: 0 = game sound, 1 = music

// results of the last dive (shown on the summary screen)
extern bool lastSuccess;
extern int  lastLoot, lastDepthBonus, lastSurvival, lastRareBonus, lastScore;
extern int  lastBag[5];