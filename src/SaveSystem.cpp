#include "SaveSystem.h"
#include <fstream>
#include "Globals.h"
#include "Constants.h"

void SaveGame() {
    std::ofstream f("save.txt");
    f << save.money << " " << save.totalEarned << " " << save.highScore << "\n";
    for (int i = 0; i < UP_COUNT; i++) f << save.level[i] << " ";
    f << "\n" << save.sfxVol << " " << save.musicVol << " " << save.waterVol << "\n";
}

void LoadGame() {
    std::ifstream f("save.txt");
    if (!f) return;                         // no save yet = fresh game
    f >> save.money >> save.totalEarned >> save.highScore;
    for (int i = 0; i < UP_COUNT; i++) f >> save.level[i];
    // Volume settings were added later: fall back to defaults for old saves.
    if (!(f >> save.sfxVol >> save.musicVol)) { save.sfxVol = 80; save.musicVol = 50; }
    if (!(f >> save.waterVol)) save.waterVol = 40;
    if (save.totalEarned >= WIN_TARGET) hasWon = true;
}