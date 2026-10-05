#pragma once
// -------------------------------------------------------------
// DIVE LOGIC  -  everything that happens between jumping off the
// boat and either surfacing safely or dying.
// -------------------------------------------------------------
void StartDive();           // builds a fresh random ocean and resets the diver
void UpdateDive(float dt);  // runs one frame of movement/oxygen/collisions
void EndDive(bool success); // scores the dive, sells loot (or loses it) and saves