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
void UpdateAudio();                             // call once per frame (streams music + water)
void StartMusic();                              // begin the looping background music
void StopMusic();                               // stop the background music
void StartWater();                              // begin the looping underwater ambience
void StopWater();                               // stop the underwater ambience
void SetAudioMuted(bool muted);
bool IsAudioMuted();
void ToggleAudioMuted();

// Independent volume controls (0.0 .. 1.0), changeable from the settings screen.
void  SetSfxVolume(float volume);
float GetSfxVolume();
void  SetBgmVolume(float volume);    // background music
float GetBgmVolume();
void  SetWaterVolume(float volume);  // underwater ambience
float GetWaterVolume();
