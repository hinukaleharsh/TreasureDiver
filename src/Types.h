#pragma once
#include "raylib.h"
#include "Constants.h"

// -------------------------------------------------------------
// TYPES  -  the "nouns" of the game: treasure, hazards, the diver
// -------------------------------------------------------------

struct TreasureType {
    const char* name;
    int   value;
    float minDepth;   // minimum depth (metres) this treasure can appear at
    Color color;
    float radius;
};

// Deeper = more valuable. Index 0..4 is used everywhere as the "type id".
inline const TreasureType TYPES[5] = {
    {"Coin",          10,   0, GOLD,     8},
    {"Pearl",         25, 100, RAYWHITE, 10},
    {"Golden Idol",   75, 250, ORANGE,   13},
    {"Ancient Relic", 150, 400, PURPLE,  14},
    {"Lost Crown",    500, 550, RED,     16},
};

enum State   { SHOP, DIVING, SUMMARY, WON };
enum Upgrade { UP_OXYGEN, UP_FINS, UP_BAG, UP_LIGHT, UP_HARPOON, UP_COUNT };

struct UpgradeDef {
    const char* name;
    const char* desc;
    int baseCost;
    int maxLevel;
};

inline const UpgradeDef UPGRADES[UP_COUNT] = {
    {"Oxygen Tank",  "+25 oxygen",              100, 6},
    {"Faster Fins",  "+15% swim speed",         150, 5},
    {"Treasure Bag", "+3 bag slots",            120, 5},
    {"Flashlight",   "see farther in the dark", 200, 4},
    {"Harpoon",      "+3 harpoons (SPACE)",     250, 3},
};

struct Treasure { Vector2 pos; int type; bool taken; };
struct Shark    { Vector2 pos; float baseY; float vx; float phase; bool alive; };
struct Jelly    { Vector2 pos; float phase; };
struct Mine     { Vector2 pos; bool active; };
struct Current  { Rectangle area; Vector2 push; };          // push = pixels/second
struct Harpoon  { Vector2 pos; float dir; bool active; };

struct Diver {
    Vector2 pos;
    float facing;      // +1 = right, -1 = left
    float oxygen;
    float health;
    float slowTimer;   // > 0 when stung by a jellyfish
    float hitTimer;    // > 0 = temporarily invulnerable after a hit
    bool  hasDived;    // becomes true once we've left the boat area
};

struct SaveData {
    int money       = 0;
    int totalEarned = 0;
    int highScore   = 0;
    int level[UP_COUNT] = {0, 0, 0, 0, 0};
};
