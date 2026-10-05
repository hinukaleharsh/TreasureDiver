#include "Dive.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include "raylib.h"
#include "Globals.h"
#include "Helpers.h"
#include "SaveSystem.h"
#include "Constants.h"

void StartDive() {
    treasures.clear(); sharks.clear(); jellies.clear();
    mines.clear(); currents.clear(); harpoons.clear();
    for (int i = 0; i < 5; i++) bag[i] = 0;

    maxOxygen       = BASE_OXYGEN + 25.0f * save.level[UP_OXYGEN];
    harpoonsLeft    = 3 * save.level[UP_HARPOON];
    maxDepthReached = 0;
    messageTimer = 0; flashTimer = 0;

    diver = Diver{Vector2{WORLD_W / 2, 30}, 1.0f, maxOxygen, 100.0f, 0.0f, 0.0f, false};

    // --- Random treasure generation: new layout every dive ---
    for (int i = 0; i < TREASURE_COUNT; i++) {
        float y = RandF(120, WORLD_H - 30);
        float d = DepthM(y);
        int type = 0;
        for (int k = 0; k < 5; k++) if (d >= TYPES[k].minDepth) type = k;
        treasures.push_back(Treasure{Vector2{RandF(30, WORLD_W - 30), y}, type, false});
    }

    // --- Difficulty progression ---
    // sqrt(random) pushes more hazards toward the bottom of the sea.
    for (int i = 0; i < 18; i++) {                       // sharks: from 100 m
        float y = 500 + (WORLD_H - 560) * sqrtf(RandF(0, 1));
        float speed = 45.0f + 0.08f * DepthM(y);         // deeper = faster
        float dir = (rand() % 2) ? 1.0f : -1.0f;
        sharks.push_back(Shark{Vector2{RandF(50, WORLD_W - 50), y}, y, speed * dir, RandF(0, 6.28f), true});
    }
    for (int i = 0; i < 25; i++) {                       // jellyfish: from 50 m
        float y = 250 + (WORLD_H - 300) * sqrtf(RandF(0, 1));
        jellies.push_back(Jelly{Vector2{RandF(30, WORLD_W - 30), y}, RandF(0, 6.28f)});
    }
    for (int i = 0; i < 25; i++) {                       // mines: from 250 m
        float y = 1250 + (WORLD_H - 1300) * sqrtf(RandF(0, 1));
        mines.push_back(Mine{Vector2{RandF(30, WORLD_W - 30), y}, true});
    }
    for (int i = 0; i < 8; i++) {                        // currents: from 250 m
        float y = RandF(1250, WORLD_H - 250);
        currents.push_back(Current{Rectangle{RandF(0, WORLD_W - 300), y, 300, 220},
                                   Vector2{RandF(-60, 60), RandF(20, 50)}});
    }

    state = DIVING;
}

void EndDive(bool success) {
    lastSuccess = success;
    lastLoot = BagValue();
    for (int i = 0; i < 5; i++) lastBag[i] = bag[i];
    lastDepthBonus = lastSurvival = lastRareBonus = lastScore = 0;

    if (success) {
        lastDepthBonus = (int)(maxDepthReached / 10.0f);      // 400 m -> +40
        lastSurvival   = 20;
        lastRareBonus  = bag[3] * 25 + bag[4] * 100;          // relics & crowns
        lastScore      = lastLoot + lastDepthBonus + lastSurvival + lastRareBonus;

        save.money       += lastLoot;                          // "sell" the treasure
        save.totalEarned += lastLoot;
        if (lastScore > save.highScore) save.highScore = lastScore;
    }
    // if we died, nothing is added: the loot is simply lost.

    SaveGame();
    if (!hasWon && save.totalEarned >= WIN_TARGET) { hasWon = true; state = WON; }
    else state = SUMMARY;
}

void UpdateDive(float dt) {
    float depthM   = DepthM(diver.pos.y);
    float pressure = Clamp01(depthM / MAX_DEPTH_M);        // 0 at surface, 1 at bottom
    maxDepthReached = std::max(maxDepthReached, depthM);

    // ----- Movement -----
    float dx = 0, dy = 0;
    if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) dx -= 1;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) dx += 1;
    if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) dy -= 1;
    if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) dy += 1;

    bool moving   = (dx != 0 || dy != 0);
    bool boosting = moving && IsKeyDown(KEY_LEFT_SHIFT) && diver.oxygen > 0;

    float speed = BASE_SPEED * (1.0f + 0.15f * save.level[UP_FINS]);
    speed *= (1.0f - 0.35f * pressure);                    // PRESSURE: slower when deep
    if (diver.slowTimer > 0) { speed *= 0.4f; diver.slowTimer -= dt; }
    if (boosting) speed *= 1.6f;

    if (moving) {
        float len = sqrtf(dx * dx + dy * dy);              // normalise so diagonals aren't faster
        diver.pos.x += dx / len * speed * dt;
        diver.pos.y += dy / len * speed * dt;
        if (dx != 0) diver.facing = dx > 0 ? 1.0f : -1.0f;
    }

    // ----- Currents -----
    for (auto& c : currents)
        if (CheckCollisionPointRec(diver.pos, c.area)) {
            diver.pos.x += c.push.x * dt;
            diver.pos.y += c.push.y * dt;
        }

    diver.pos.x = std::max(15.0f, std::min(WORLD_W - 15.0f, diver.pos.x));
    diver.pos.y = std::max(10.0f, std::min(WORLD_H - 15.0f, diver.pos.y));
    if (diver.pos.y > 120) diver.hasDived = true;

    // ----- Oxygen (PRESSURE: drains faster when deep) -----
    float drain = 1.0f + 1.5f * pressure;
    if (boosting) drain += 3.0f;
    diver.oxygen -= drain * dt;
    if (diver.oxygen <= 0) { diver.oxygen = 0; diver.health -= 8.0f * dt; }

    if (diver.hitTimer > 0)  diver.hitTimer  -= dt;
    if (messageTimer > 0)    messageTimer    -= dt;
    if (flashTimer > 0)      flashTimer      -= dt;

    // ----- Collect treasure -----
    for (auto& t : treasures) {
        if (t.taken) continue;
        if (CheckCollisionCircles(diver.pos, 14, t.pos, TYPES[t.type].radius)) {
            if (BagCount() >= BagCapacity()) {
                ShowMessage("Bag is full! Head up and sell.", 1.0f);
            } else {
                t.taken = true;
                bag[t.type]++;
                ShowMessage(std::string("+ ") + TYPES[t.type].name + "  ($" + std::to_string(TYPES[t.type].value) + ")");
            }
        }
    }

    // ----- Sharks -----
    float now = (float)GetTime();
    for (auto& s : sharks) {
        if (!s.alive) continue;
        s.pos.x += s.vx * dt;
        if (s.pos.x < 30 || s.pos.x > WORLD_W - 30) s.vx = -s.vx;
        s.pos.y = s.baseY + sinf(now * 1.2f + s.phase) * 35.0f;

        if (diver.hitTimer <= 0 && CheckCollisionCircles(diver.pos, 12, s.pos, 24)) {
            diver.health -= 25; diver.hitTimer = 1.5f; flashTimer = 0.3f;
            ShowMessage("Shark bite!", 1.0f);
        }
    }

    // ----- Jellyfish -----
    for (auto& j : jellies) {
        Vector2 p{j.pos.x, j.pos.y + sinf(now + j.phase) * 10.0f};
        if (CheckCollisionCircles(diver.pos, 12, p, 14)) {
            if (diver.slowTimer <= 0) ShowMessage("Stung! You are slowed.", 1.5f);
            diver.slowTimer = 3.0f;
        }
    }

    // ----- Mines -----
    for (auto& m : mines) {
        if (m.active && CheckCollisionCircles(diver.pos, 12, m.pos, 12)) {
            m.active = false;
            diver.health -= 40; flashTimer = 0.5f;
            ShowMessage("BOOM! You hit a mine!", 1.5f);
        }
    }

    // ----- Harpoon -----
    if (IsKeyPressed(KEY_SPACE) && harpoonsLeft > 0) {
        harpoonsLeft--;
        harpoons.push_back(Harpoon{diver.pos, diver.facing, true});
    }
    for (auto& h : harpoons) {
        if (!h.active) continue;
        h.pos.x += h.dir * 450.0f * dt;
        if (h.pos.x < 0 || h.pos.x > WORLD_W || fabsf(h.pos.x - diver.pos.x) > 400) h.active = false;
        for (auto& s : sharks)
            if (s.alive && h.active && CheckCollisionCircles(h.pos, 6, s.pos, 24)) {
                s.alive = false; h.active = false;
                ShowMessage("Shark defeated!", 1.0f);
            }
    }

    // ----- End conditions -----
    if (diver.health <= 0)                         EndDive(false);   // died: loot lost
    else if (diver.hasDived && diver.pos.y <= 25)  EndDive(true);    // back on the boat
}