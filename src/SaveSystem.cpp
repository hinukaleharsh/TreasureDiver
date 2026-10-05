#include "SaveSystem.h"
#include <fstream>
#include "Globals.h"
#include "Constants.h"

void SaveGame() {
    std::ofstream f("save.txt");
    f << save.money << " " << save.totalEarned << " " << save.highScore << "\n";
    for (int i = 0; i < UP_COUNT; i++) f << save.level[i] << " ";
}

void LoadGame() {
    std::ifstream f("save.txt");
    if (!f) return;                         // no save yet = fresh game
    f >> save.money >> save.totalEarned >> save.highScore;
    for (int i = 0; i < UP_COUNT; i++) f >> save.level[i];
    if (save.totalEarned >= WIN_TARGET) hasWon = true;
}