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
    SetSfxVolume(save.sfxVol / 100.0f);
    SetBgmVolume(save.musicVol / 100.0f);
    SetWaterVolume(save.waterVol / 100.0f);
    StartMusic();

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
                if (IsKeyPressed(KEY_O)) { PlaySfx(SFX_MENU); state = SETTINGS; }
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
            case SETTINGS: {
                if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) { settingsSel = (settingsSel + 2) % 3; PlaySfx(SFX_MENU); }
                if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) { settingsSel = (settingsSel + 1) % 3; PlaySfx(SFX_MENU); }

                static float adjustTimer = 0;
                int dir = 0;
                if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) dir -= 1;
                if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) dir += 1;
                if (dir != 0) {
                    adjustTimer -= dt;              // first tap applies at once, then repeats while held
                    if (adjustTimer <= 0) {
                        int* level = (settingsSel == 0) ? &save.sfxVol
                                   : (settingsSel == 1) ? &save.musicVol
                                   :                      &save.waterVol;
                        *level += dir * 5;
                        if (*level < 0)   *level = 0;
                        if (*level > 100) *level = 100;

                        if (settingsSel == 0)      SetSfxVolume(save.sfxVol / 100.0f);
                        else if (settingsSel == 1) SetBgmVolume(save.musicVol / 100.0f);
                        else                       SetWaterVolume(save.waterVol / 100.0f);

                        PlaySfx(SFX_MENU);
                        adjustTimer = 0.08f;
                    }
                } else {
                    adjustTimer = 0.0f;
                }

                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_O)) {
                    SaveGame(); state = SHOP; PlaySfx(SFX_MENU);
                }
                break;
            }
        }

        // ---- DRAW ----
        BeginDrawing();
        ClearBackground(BLACK);
        switch (state) {
            case SHOP:     DrawShop();     break;
            case DIVING:   DrawDive();     break;
            case SUMMARY:  DrawSummary();  break;
            case WON:      DrawWon();      break;
            case SETTINGS: DrawSettings(); break;
        }
        EndDrawing();
    }

    CloseAudio();
    CloseWindow();
    return 0;
}