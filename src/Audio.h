#pragma once
// -------------------------------------------------------------
// AUDIO  -  every sound in the game is synthesised in C++ at
// runtime, so no external .wav / .ogg files are required.
// The module builds a small library of sound effects plus a
// looping underwater ambience and plays them on demand.
// -------------------------------------------------------------

enum SfxId {
    SFX_COLLECT,      // treasure picked up
    SFX_BAG_FULL,     // tried to grab treasure with a full bag
    SFX_HURT,         // bitten by a shark
    SFX_STING,        // stung by a jellyfish
    SFX_EXPLODE,      // mine detonation
    SFX_HARPOON,      // harpoon fired
    SFX_SHARK_HIT,    // shark defeated
    SFX_BUY,          // upgrade purchased
    SFX_DENIED,       // purchase refused
    SFX_DIVE,         // splash: the dive begins
    SFX_SURFACE,      // safe return to the boat
    SFX_DEATH,        // the diver dies
    SFX_WIN,          // victory fanfare
    SFX_LOW_OXYGEN,   // oxygen warning beep
    SFX_MENU,         // UI tick
    SFX_COUNT
};

void InitAudio();                               // open the device and build every sound
void CloseAudio();                              // free sounds and close the device
bool AudioReady();                              // true once a device is available
void PlaySfx(SfxId id, float pitch = 1.0f);     // play an effect (pitch 1.0 = normal)
void UpdateAudio();                             // call once per frame (streams the ambience)
void StartAmbient();                            // begin the looping underwater ambience
void StopAmbient();                             // stop the ambience
void SetAudioMuted(bool muted);
bool IsAudioMuted();
void ToggleAudioMuted();
