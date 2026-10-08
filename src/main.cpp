// =====================================================================
//  TREASURE DIVER  -  a risk-vs-reward diving game
//  Language: C++     Library: raylib (graphics, keyboard, window)
//
//  This file is deliberately short: it just runs the loop and calls
//  out to the other files. Look there for how everything works.
// =====================================================================
#include "raylib.h"
#include <cstdlib>
#include <ctime>
#include "Constants.h"
#include "Types.h"
#include "Globals.h"
#include "Helpers.h"
#include "SaveSystem.h"
#include "Dive.h"
#include "Draw.h"
#include "Audio.h"

int main() {
    InitWindow(SCREEN_W, SCREEN_H, "Treasure Diver");
    SetTargetFPS(60);
    srand((unsigned)time(nullptr));
    InitAudio();
    LoadGame();

    cam.offset   = Vector2{SCREEN_W / 2.0f, SCREEN_H / 2.0f};
    cam.target   = Vector2{WORLD_W / 2.0f, 0};
    cam.rotation = 0;
    cam.zoom     = 1;

    while (!WindowShouldClose()) {            // runs 60 times per second
        float dt = GetFrameTime();
        UpdateAudio();
        if (IsKeyPressed(KEY_M)) ToggleAudioMuted();

        // ---- UPDATE ----
        switch (state) {
            case SHOP:
                for (int i = 0; i < UP_COUNT; i++) {
                    if (!IsKeyPressed(KEY_ONE + i)) continue;
                    if (save.level[i] >= UPGRADES[i].maxLevel) { shopMsg = "Already at max level!"; PlaySfx(SFX_DENIED); }
                    else if (save.money < UpgradeCost(i))        { shopMsg = "Not enough money!";      PlaySfx(SFX_DENIED); }
                    else {
                        save.money -= UpgradeCost(i);
                        save.level[i]++;
                        SaveGame();
                        shopMsg = std::string("Bought ") + UPGRADES[i].name + "!";
                        PlaySfx(SFX_BUY);
                    }
                }
                if (IsKeyPressed(KEY_ENTER)) StartDive();
                break;
            case DIVING:
                UpdateDive(dt);
                cam.target.y = diver.pos.y;   // camera follows the diver
                break;
            case SUMMARY:
                if (IsKeyPressed(KEY_ENTER)) { PlaySfx(SFX_MENU); shopMsg = ""; state = SHOP; }
                break;
            case WON:
                if (IsKeyPressed(KEY_ENTER)) { PlaySfx(SFX_MENU); shopMsg = ""; state = SHOP; }
                if (IsKeyPressed(KEY_R)) { save = SaveData(); hasWon = false; SaveGame(); shopMsg = ""; state = SHOP; }
                break;
        }

        // ---- DRAW ----
        BeginDrawing();
        ClearBackground(BLACK);
        switch (state) {
            case SHOP:    DrawShop();    break;
            case DIVING:  DrawDive();    break;
            case SUMMARY: DrawSummary(); break;
            case WON:     DrawWon();     break;
        }
        EndDrawing();
    }

    CloseAudio();
    CloseWindow();
    return 0;
}