#include "SaveSystem.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>

EM_JS(int, LoadStoredValue, (int position), {
    const value = localStorage.getItem("treasure-diver-" + position);
    if (value === null) return 0;
    const parsed = Number.parseInt(value, 10);
    return Number.isFinite(parsed) ? parsed : 0;
});

EM_JS(void, SaveStoredValue, (int position, int value), {
    localStorage.setItem("treasure-diver-" + position, String(value));
});
#else
#include <fstream>
#endif
#include "Globals.h"
#include "Constants.h"

void SaveGame() {
#ifdef __EMSCRIPTEN__
    SaveStoredValue(0, save.money);
    SaveStoredValue(1, save.totalEarned);
    SaveStoredValue(2, save.highScore);
    for (int i = 0; i < UP_COUNT; i++) SaveStoredValue(3 + i, save.level[i]);
#else
    std::ofstream f("save.txt");
    f << save.money << " " << save.totalEarned << " " << save.highScore << "\n";
    for (int i = 0; i < UP_COUNT; i++) f << save.level[i] << " ";
#endif
}

void LoadGame() {
#ifdef __EMSCRIPTEN__
    save.money = LoadStoredValue(0);
    save.totalEarned = LoadStoredValue(1);
    save.highScore = LoadStoredValue(2);
    for (int i = 0; i < UP_COUNT; i++) save.level[i] = LoadStoredValue(3 + i);
#else
    std::ifstream f("save.txt");
    if (!f) return;                         // no save yet = fresh game
    f >> save.money >> save.totalEarned >> save.highScore;
    for (int i = 0; i < UP_COUNT; i++) f >> save.level[i];
#endif
    if (save.totalEarned >= WIN_TARGET) hasWon = true;
}