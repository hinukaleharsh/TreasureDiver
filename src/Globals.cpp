#include "Globals.h"
#include "Constants.h"

SaveData save;
State    state = SHOP;

Diver    diver;
Camera2D cam;
std::vector<Treasure> treasures;
std::vector<Shark>    sharks;
std::vector<Jelly>    jellies;
std::vector<Mine>     mines;
std::vector<Current>  currents;
std::vector<Harpoon>  harpoons;

int   bag[5] = {0, 0, 0, 0, 0};
float maxDepthReached = 0;
int   harpoonsLeft    = 0;
float maxOxygen       = BASE_OXYGEN;

std::string message;
float messageTimer = 0;
float flashTimer   = 0;
std::string shopMsg;
bool  hasWon = false;
int   settingsSel = 0;

bool lastSuccess = false;
int  lastLoot = 0, lastDepthBonus = 0, lastSurvival = 0, lastRareBonus = 0, lastScore = 0;
int  lastBag[5] = {0, 0, 0, 0, 0};