#pragma once
#include <string>
#include "raylib.h"

// -------------------------------------------------------------
// HELPERS  -  small reusable tools used by several files
// -------------------------------------------------------------

float RandF(float a, float b);     // random float between a and b
float Clamp01(float v);            // squeeze v into the 0..1 range
float DepthM(float y);             // convert a pixel Y position to metres

int BagCapacity();                 // how many items the bag can hold right now
int BagCount();                    // how many items are in the bag right now
int BagValue();                    // total $ value of everything in the bag
int UpgradeCost(int upgradeIndex); // $ price of the next level of an upgrade

Color LerpColor(Color a, Color b, float t);  // blend two colors
Color WaterColor(float depthM);              // water color at a given depth

void ShowMessage(const std::string& s, float secs = 2.0f); // pop-up text on screen
const char* LayerName(float depthM);                        // "Layer 2: Shark Waters" etc.

float VisionRadius(float depthM);  // how far the diver can see (pressure + flashlight)
bool  CanSee(Vector2 worldPos);    // is a point within the diver's vision?