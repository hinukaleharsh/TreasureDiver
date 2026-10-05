#pragma once
// -------------------------------------------------------------
// CONSTANTS  -  change these numbers to rebalance the whole game
// -------------------------------------------------------------
const int   SCREEN_W       = 960;
const int   SCREEN_H       = 640;
const float WORLD_W        = 960.0f;   // the sea is as wide as the window
const float WORLD_H        = 3500.0f;  // and 3500 pixels deep...
const float PX_PER_METER   = 5.0f;     // ...which is 700 metres (5 px = 1 m)
const float MAX_DEPTH_M    = WORLD_H / PX_PER_METER;
const int   WIN_TARGET     = 3000;     // total money to earn to win
const int   TREASURE_COUNT = 90;       // treasures generated per dive
const int   START_BAG      = 5;        // bag slots before upgrades
const float BASE_SPEED     = 100.0f;   // swim speed in pixels/second
const float BASE_OXYGEN    = 100.0f;