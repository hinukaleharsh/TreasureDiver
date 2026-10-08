#include "Draw.h"
#include <cmath>
#include <algorithm>
#include "raylib.h"
#include "Globals.h"
#include "Helpers.h"
#include "Audio.h"

// ----- small internal helpers, only used inside this file -----
static void DrawBoat(float x, float y) {
    DrawRectangle((int)x - 70, (int)y - 25, 140, 25, MAROON);        // hull
    DrawTriangle(Vector2{x - 70, y}, Vector2{x + 70, y}, Vector2{x + 50, y + 15}, MAROON);
    DrawRectangle((int)x - 20, (int)y - 55, 40, 30, RAYWHITE);       // cabin
    DrawRectangle((int)x - 12, (int)y - 48, 12, 12, SKYBLUE);
}

static void DrawSharkShape(const Shark& s) {
    float dir = s.vx > 0 ? 1.0f : -1.0f;
    int x = (int)s.pos.x, y = (int)s.pos.y;
    DrawEllipse(x, y, 26, 10, GRAY);
    DrawPoly(Vector2{s.pos.x - dir * 28, s.pos.y}, 3, 12, dir > 0 ? 180.0f : 0.0f, GRAY);   // tail
    DrawPoly(Vector2{s.pos.x, s.pos.y - 11}, 3, 8, 270.0f, DARKGRAY);                          // fin
    DrawCircle(x + (int)(dir * 16), y - 3, 2, BLACK);                                          // eye
}

void DrawDive() {
    float depthM   = DepthM(diver.pos.y);
    float pressure = Clamp01(depthM / MAX_DEPTH_M);

    // Background: colour of the water at the top and bottom of the screen
    float topD = DepthM(cam.target.y - SCREEN_H / 2.0f);
    float botD = DepthM(cam.target.y + SCREEN_H / 2.0f);
    DrawRectangleGradientV(0, 0, SCREEN_W, SCREEN_H, WaterColor(topD), WaterColor(botD));

    // ---------- world-space drawing (moves with the camera) ----------
    BeginMode2D(cam);

    DrawRectangle(0, -900, (int)WORLD_W, 900, Color{150, 210, 245, 255});     // sky
    DrawBoat(WORLD_W / 2, 0);
    DrawRectangle(0, (int)WORLD_H, (int)WORLD_W, 400, Color{60, 45, 30, 255});   // sea floor

    for (int m = 100; m <= (int)MAX_DEPTH_M; m += 100) {                        // depth markers
        int y = (int)(m * PX_PER_METER);
        DrawLine(0, y, (int)WORLD_W, y, Fade(WHITE, 0.12f));
        DrawText(TextFormat("%dm", m), 6, y + 3, 16, Fade(WHITE, 0.5f));
    }

    for (auto& c : currents) {                                                   // currents
        DrawRectangleRec(c.area, Fade(WHITE, 0.06f));
        Vector2 mid{c.area.x + c.area.width / 2, c.area.y + c.area.height / 2};
        float len = sqrtf(c.push.x * c.push.x + c.push.y * c.push.y);
        DrawLineEx(mid, Vector2{mid.x + c.push.x / len * 60, mid.y + c.push.y / len * 60}, 3, Fade(WHITE, 0.4f));
        DrawCircleV(Vector2{mid.x + c.push.x / len * 60, mid.y + c.push.y / len * 60}, 6, Fade(WHITE, 0.4f));
    }

    float now = (float)GetTime();
    for (auto& t : treasures) {                                                  // treasure
        if (t.taken || !CanSee(t.pos)) continue;
        float r = TYPES[t.type].radius + sinf(now * 4 + t.pos.x) * 1.5f;
        DrawCircleV(t.pos, r, TYPES[t.type].color);
        DrawCircleLines((int)t.pos.x, (int)t.pos.y, r + 2, Fade(WHITE, 0.6f));
    }
    for (auto& m : mines) {                                                      // mines
        if (!m.active || !CanSee(m.pos)) continue;
        for (int a = 0; a < 8; a++) {
            float ang = a * 0.785f;
            DrawLine((int)m.pos.x, (int)m.pos.y, (int)(m.pos.x + cosf(ang) * 18), (int)(m.pos.y + sinf(ang) * 18), DARKGRAY);
        }
        DrawCircleV(m.pos, 12, DARKGRAY);
        DrawCircleV(m.pos, 4, ((int)(now * 3) % 2) ? RED : MAROON);
    }
    for (auto& j : jellies) {                                                    // jellyfish
        Vector2 p{j.pos.x, j.pos.y + sinf(now + j.phase) * 10.0f};
        if (!CanSee(p)) continue;
        DrawCircleSector(p, 14, 180, 360, 16, Fade(PINK, 0.85f));
        for (int k = -8; k <= 8; k += 4) DrawLine((int)p.x + k, (int)p.y, (int)p.x + k, (int)p.y + 14, Fade(PINK, 0.7f));
    }
    for (auto& s : sharks)                                                       // sharks
        if (s.alive && CanSee(s.pos)) DrawSharkShape(s);
    for (auto& h : harpoons)                                                     // harpoons
        if (h.active) DrawLineEx(h.pos, Vector2{h.pos.x - h.dir * 20, h.pos.y}, 3, LIGHTGRAY);

    // Diver (blinks while invulnerable)
    if (diver.hitTimer <= 0 || ((int)(now * 12) % 2)) {
        DrawCircleV(diver.pos, 12, diver.slowTimer > 0 ? PINK : ORANGE);
        DrawRectangle((int)(diver.pos.x - diver.facing * 16 - 3), (int)diver.pos.y - 6, 6, 12, DARKBLUE);  // tank
        DrawCircleV(Vector2{diver.pos.x + diver.facing * 6, diver.pos.y - 2}, 5, SKYBLUE);                 // mask
    }
    EndMode2D();

    // ---------- PRESSURE: darkness + limited vision ----------
    Vector2 scr = GetWorldToScreen2D(diver.pos, cam);
    float vis   = VisionRadius(depthM);
    float alpha = 0.95f * Clamp01((depthM - 40.0f) / 360.0f);
    DrawRing(scr, vis * 0.75f, vis, 0, 360, 64, Fade(BLACK, alpha * 0.5f));      // soft edge
    DrawRing(scr, vis, 1800, 0, 360, 64, Fade(BLACK, alpha));                    // darkness
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.25f * pressure));      // overall tint
    if (flashTimer > 0) DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(RED, flashTimer));

    // ---------- HUD ----------
    DrawRectangle(10, 10, 250, 100, Fade(BLACK, 0.55f));
    DrawText("OXYGEN", 18, 16, 14, WHITE);
    DrawRectangle(90, 16, 160, 14, Fade(WHITE, 0.2f));
    DrawRectangle(90, 16, (int)(160 * diver.oxygen / maxOxygen), 14, diver.oxygen / maxOxygen > 0.25f ? SKYBLUE : RED);
    DrawText("HEALTH", 18, 38, 14, WHITE);
    DrawRectangle(90, 38, 160, 14, Fade(WHITE, 0.2f));
    DrawRectangle(90, 38, (int)(160 * diver.health / 100.0f), 14, LIME);
    DrawText(TextFormat("Depth: %d m", (int)depthM), 18, 62, 18, WHITE);
    DrawText(TextFormat("Harpoons: %d", harpoonsLeft), 18, 86, 14, LIGHTGRAY);

    DrawText(LayerName(depthM), SCREEN_W / 2 - MeasureText(LayerName(depthM), 20) / 2, 12, 20, WHITE);

    // Inventory panel
    DrawRectangle(SCREEN_W - 230, 10, 220, 160, Fade(BLACK, 0.55f));
    DrawText(TextFormat("BAG  %d / %d", BagCount(), BagCapacity()), SCREEN_W - 220, 16, 16, WHITE);
    for (int i = 0; i < 5; i++) {
        DrawCircle(SCREEN_W - 214, 48 + i * 22, 6, TYPES[i].color);
        DrawText(TextFormat("%s x%d", TYPES[i].name, bag[i]), SCREEN_W - 200, 41 + i * 22, 15, LIGHTGRAY);
    }
    DrawText(TextFormat("Loot: $%d", BagValue()), SCREEN_W - 220, 152 - 8, 16, GOLD);

    if (diver.oxygen <= 0)
        DrawText("OUT OF OXYGEN! SWIM UP!", SCREEN_W / 2 - 170, 60, 24, RED);
    else if (diver.hasDived && diver.oxygen < maxOxygen * 0.25f)
        DrawText("Low oxygen!", SCREEN_W / 2 - 60, 60, 22, ORANGE);
    if (messageTimer > 0)
        DrawText(message.c_str(), SCREEN_W / 2 - MeasureText(message.c_str(), 22) / 2, SCREEN_H - 60, 22, WHITE);

    DrawText("Arrows/WASD: swim   SHIFT: boost   SPACE: harpoon   Surface = sell   M: mute", 10, SCREEN_H - 22, 14, Fade(WHITE, 0.6f));
    DrawText(IsAudioMuted() ? "SOUND: OFF" : "SOUND: ON", SCREEN_W - 100, SCREEN_H - 22, 14, IsAudioMuted() ? GRAY : LIME);
}

void DrawShop() {
    DrawRectangleGradientV(0, 0, SCREEN_W, SCREEN_H, Color{130, 200, 240, 255}, Color{10, 50, 100, 255});
    DrawRectangle(0, 120, SCREEN_W, 4, Fade(WHITE, 0.5f));
    DrawBoat(800, 120);

    DrawText("TREASURE DIVER", 40, 24, 44, WHITE);
    DrawText(TextFormat("Money: $%d", save.money), 40, 84, 26, GOLD);
    DrawText(TextFormat("High score: %d", save.highScore), 260, 90, 20, WHITE);
    DrawText("[O] Audio settings", 540, 90, 20, LIGHTGRAY);

    DrawText(TextFormat("Goal: earn $%d in total  (%d / %d)", WIN_TARGET, save.totalEarned, WIN_TARGET), 40, 140, 18, WHITE);
    DrawRectangle(40, 166, 420, 14, Fade(BLACK, 0.4f));
    DrawRectangle(40, 166, (int)(420 * Clamp01(save.totalEarned / (float)WIN_TARGET)), 14, LIME);

    DrawText("UPGRADE SHOP  (press 1-5 to buy)", 40, 205, 22, WHITE);
    for (int i = 0; i < UP_COUNT; i++) {
        int y = 245 + i * 62;
        DrawRectangle(40, y, 880, 54, Fade(BLACK, 0.4f));
        DrawText(TextFormat("[%d] %s", i + 1, UPGRADES[i].name), 55, y + 6, 22, WHITE);
        DrawText(UPGRADES[i].desc, 55, y + 32, 16, LIGHTGRAY);
        DrawText(TextFormat("Lv %d/%d", save.level[i], UPGRADES[i].maxLevel), 520, y + 15, 22, SKYBLUE);
        if (save.level[i] >= UPGRADES[i].maxLevel) DrawText("MAX", 750, y + 15, 22, LIME);
        else DrawText(TextFormat("$%d", UpgradeCost(i)), 750, y + 15, 22, save.money >= UpgradeCost(i) ? GOLD : GRAY);
    }
    DrawText(shopMsg.c_str(), 40, 565, 20, YELLOW);
    DrawText("Press ENTER to DIVE!", 40, 598, 28, WHITE);
    DrawText(IsAudioMuted() ? "SOUND: OFF (M)" : "SOUND: ON (M)", SCREEN_W - 220, 598, 18, IsAudioMuted() ? GRAY : LIME);
}

void DrawSummary() {
    ClearBackground(Color{10, 30, 60, 255});
    DrawText(lastSuccess ? "DIVE COMPLETE!" : "YOU DIED - LOOT LOST", 60, 40, 40, lastSuccess ? LIME : RED);

    int y = 120;
    for (int i = 0; i < 5; i++) {
        if (lastBag[i] == 0) continue;
        DrawText(TextFormat("%s x%d  =  $%d", TYPES[i].name, lastBag[i], lastBag[i] * TYPES[i].value), 80, y, 24, WHITE);
        y += 34;
    }
    if (y == 120) { DrawText("(no treasure collected)", 80, y, 24, GRAY); y += 34; }

    y += 20;
    if (lastSuccess) {
        DrawText(TextFormat("Treasure value:   %d", lastLoot),      80, y,       22, GOLD);
        DrawText(TextFormat("Depth bonus:      +%d", lastDepthBonus), 80, y + 30,  22, WHITE);
        DrawText(TextFormat("Survival bonus:   +%d", lastSurvival),   80, y + 60,  22, WHITE);
        DrawText(TextFormat("Rare bonus:       +%d", lastRareBonus),  80, y + 90,  22, WHITE);
        DrawText(TextFormat("DIVE SCORE:       %d", lastScore),       80, y + 130, 28, LIME);
        DrawText(TextFormat("Sold for $%d.   Best score: %d", lastLoot, save.highScore), 80, y + 175, 20, LIGHTGRAY);
    } else {
        DrawText(TextFormat("You lost $%d of treasure.", lastLoot), 80, y, 24, RED);
        DrawText("Tip: head back up BEFORE your oxygen runs low!", 80, y + 40, 20, LIGHTGRAY);
    }
    DrawText("Press ENTER to go to the shop", 80, SCREEN_H - 60, 24, WHITE);
}

void DrawWon() {
    ClearBackground(Color{5, 40, 60, 255});
    DrawText("YOU WIN!", 60, 100, 70, GOLD);
    DrawText(TextFormat("You earned $%d in treasure. You are a legendary diver!", save.totalEarned), 60, 210, 24, WHITE);
    DrawText(TextFormat("Best dive score: %d", save.highScore), 60, 260, 24, LIME);
    DrawText("ENTER = keep playing        R = reset save and start over", 60, 380, 22, LIGHTGRAY);
}

static void DrawVolumeRow(int y, const char* label, int value, bool selected) {
    DrawRectangle(60, y, SCREEN_W - 120, 72, selected ? Fade(SKYBLUE, 0.22f) : Fade(BLACK, 0.35f));
    if (selected) DrawRectangleLines(60, y, SCREEN_W - 120, 72, SKYBLUE);

    DrawText(label, 84, y + 10, 24, selected ? WHITE : LIGHTGRAY);

    int sx = 84, sy = y + 46, sw = SCREEN_W - 280, sh = 14;
    DrawRectangle(sx, sy, sw, sh, Fade(WHITE, 0.15f));
    int fill = (int)(sw * (value / 100.0f));
    DrawRectangle(sx, sy, fill, sh, selected ? LIME : GREEN);
    DrawCircle(sx + fill, sy + sh / 2, 9, WHITE);

    DrawText(TextFormat("%d%%", value), sx + sw + 28, y + 34, 26, selected ? GOLD : GRAY);
}

void DrawSettings() {
    DrawRectangleGradientV(0, 0, SCREEN_W, SCREEN_H, Color{20, 32, 62, 255}, Color{4, 8, 20, 255});
    DrawText("AUDIO SETTINGS", 60, 50, 44, WHITE);
    DrawRectangle(60, 104, SCREEN_W - 120, 3, Fade(WHITE, 0.5f));

    DrawVolumeRow(140, "Game Sound Effects", save.sfxVol,   settingsSel == 0);
    DrawVolumeRow(230, "Background Music",   save.musicVol, settingsSel == 1);
    DrawVolumeRow(320, "Water Ambience",     save.waterVol, settingsSel == 2);

    DrawText(IsAudioMuted() ? "All sound is currently MUTED" : "Sound is ON",
             60, 416, 22, IsAudioMuted() ? RED : LIME);

    DrawText("UP / DOWN: choose        LEFT / RIGHT: adjust        M: mute all",
             60, SCREEN_H - 100, 20, LIGHTGRAY);
    DrawText("Press ENTER or O to return to the shop", 60, SCREEN_H - 62, 22, WHITE);
}