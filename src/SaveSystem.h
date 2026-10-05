#pragma once
// -------------------------------------------------------------
// SAVE SYSTEM  -  reads and writes save.txt next to the game
// -------------------------------------------------------------
void SaveGame();   // writes 'save' (money, upgrades, high score) to disk
void LoadGame();   // reads it back on startup (does nothing if no file exists yet)