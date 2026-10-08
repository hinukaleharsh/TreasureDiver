#include "Audio.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include "raylib.h"

// -------------------------------------------------------------
// This file builds audio data by hand and hands it to raylib.
//   - effects are 16-bit mono PCM Waves loaded with LoadSoundFromWave
//   - the ambience is a generated WAV held in memory and streamed as Music
// A small pool of "voice" aliases per effect lets sounds overlap.
// -------------------------------------------------------------

static const int   kRate     = 44100;
static const float kPi       = 3.14159265358979f;
static const int   kVoices   = 4;      // simultaneous copies of each effect

struct SfxVoice {
    Sound base{};
    Sound voices[kVoices]{};
    int   next = 0;
};

static SfxVoice g_sfx[SFX_COUNT];
static Music    g_music{};
static Music    g_water{};
static std::vector<unsigned char> g_musicWav;     // kept alive while the stream reads it
static std::vector<unsigned char> g_waterWav;
static bool  g_ready    = false;
static bool  g_muted    = false;
static float g_sfxVol   = 0.8f;
static float g_musicVol = 0.5f;
static float g_waterVol = 0.4f;

static inline float RandF01() { return rand() / (float)RAND_MAX; }
static inline float Clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

static void Normalize(std::vector<float>& b, float peak) {
    float m = 0.0f;
    for (float v : b) m = fmaxf(m, fabsf(v));
    if (m < 1e-6f) return;
    float s = peak / m;
    for (float& v : b) v *= s;
}

// A tiny attack/tempo-decay note used by several tonal effects.
static float Note(float t, float freq, float start, float dur) {
    float lt = t - start;
    if (lt < 0.0f || lt > dur) return 0.0f;
    float env = (1.0f - expf(-300.0f * lt)) * expf(-6.0f * lt);
    return sinf(2.0f * kPi * freq * lt) * env;
}

// Same as Note() but with a square timbre (retro blips / warnings).
static float SquareNote(float t, float freq, float start, float dur) {
    float lt = t - start;
    if (lt < 0.0f || lt > dur) return 0.0f;
    float env = (1.0f - expf(-300.0f * lt)) * expf(-9.0f * lt);
    return (sinf(2.0f * kPi * freq * lt) > 0.0f ? 1.0f : -1.0f) * env;
}

static Sound MakeSound(const std::vector<float>& buf) {
    std::vector<short> pcm(buf.size());
    for (size_t i = 0; i < buf.size(); i++)
        pcm[i] = (short)(Clampf(buf[i], -1.0f, 1.0f) * 32000.0f);

    Wave w{};
    w.frameCount = (unsigned int)pcm.size();
    w.sampleRate = kRate;
    w.sampleSize = 16;
    w.channels   = 1;
    w.data       = pcm.data();
    return LoadSoundFromWave(w);          // raylib copies the samples
}

// ------------------------- effect builders -------------------------

static std::vector<float> BuildCollect() {
    int n = (int)(kRate * 0.28f);
    std::vector<float> b(n);
    float ph = 0.0f;
    for (int i = 0; i < n; i++) {
        float t = i / (float)kRate;
        float f = 760.0f + 640.0f * (1.0f - expf(-28.0f * t));   // rising "coin" blip
        ph += 2.0f * kPi * f / kRate;
        float env = (1.0f - expf(-250.0f * t)) * expf(-9.5f * t);
        b[i] = (sinf(ph) + 0.35f * sinf(ph * 2.0f)) * env;
    }
    Normalize(b, 0.75f);
    return b;
}

static std::vector<float> BuildBagFull() {
    int n = (int)(kRate * 0.22f);
    std::vector<float> b(n);
    float ph = 0.0f;
    for (int i = 0; i < n; i++) {
        float t = i / (float)kRate;
        float f = 190.0f - 70.0f * (1.0f - expf(-25.0f * t));
        ph += 2.0f * kPi * f / kRate;
        float env = (1.0f - expf(-400.0f * t)) * expf(-12.0f * t);
        b[i] = ((sinf(ph) > 0.0f ? 0.7f : -0.7f) + sinf(ph) * 0.3f) * env;
    }
    Normalize(b, 0.6f);
    return b;
}

static std::vector<float> BuildHurt() {
    int n = (int)(kRate * 0.45f);
    std::vector<float> b(n);
    float ph = 0.0f, lp = 0.0f;
    for (int i = 0; i < n; i++) {
        float t = i / (float)kRate;
        float f = 230.0f - 165.0f * (1.0f - expf(-10.0f * t));    // falling growl
        ph += 2.0f * kPi * f / kRate;
        float env = (1.0f - expf(-120.0f * t)) * expf(-6.5f * t);
        float noise = RandF01() * 2.0f - 1.0f;
        lp += 0.15f * (noise - lp);
        b[i] = (sinf(ph) * 0.8f + lp * 0.5f + sinf(ph * 0.5f) * 0.3f) * env;
    }
    Normalize(b, 0.9f);
    return b;
}

static std::vector<float> BuildSting() {
    int n = (int)(kRate * 0.4f);
    std::vector<float> b(n);
    float ph = 0.0f;
    for (int i = 0; i < n; i++) {
        float t = i / (float)kRate;
        float f = 150.0f + 40.0f * sinf(2.0f * kPi * 3.0f * t);
        ph += 2.0f * kPi * f / kRate;
        float trem = 0.5f + 0.5f * sinf(2.0f * kPi * 32.0f * t);   // electric buzz
        float env = (1.0f - expf(-300.0f * t)) * expf(-7.0f * t);
        b[i] = (sinf(ph) * 0.6f + (RandF01() * 2.0f - 1.0f) * 0.25f) * trem * env;
    }
    Normalize(b, 0.7f);
    return b;
}

static std::vector<float> BuildExplode() {
    int n = (int)(kRate * 0.9f);
    std::vector<float> b(n);
    float ph = 0.0f, lp = 0.0f;
    for (int i = 0; i < n; i++) {
        float t = i / (float)kRate;
        float f = 95.0f - 60.0f * (1.0f - expf(-8.0f * t));        // sub-bass thump
        ph += 2.0f * kPi * f / kRate;
        float env = (1.0f - expf(-90.0f * t)) * expf(-4.2f * t);
        float noise = RandF01() * 2.0f - 1.0f;
        float cut = 0.45f * expf(-3.0f * t) + 0.02f;               // darkening noise
        lp += cut * (noise - lp);
        b[i] = (lp * 0.9f + sinf(ph) * 0.8f) * env;
    }
    Normalize(b, 0.95f);
    return b;
}

static std::vector<float> BuildHarpoon() {
    int n = (int)(kRate * 0.2f);
    std::vector<float> b(n);
    float ph = 0.0f, lp = 0.0f;
    for (int i = 0; i < n; i++) {
        float t = i / (float)kRate;
        float f = 320.0f + 900.0f * (t / 0.2f);                    // whoosh sweep
        ph += 2.0f * kPi * f / kRate;
        float noise = RandF01() * 2.0f - 1.0f;
        lp += 0.3f * (noise - lp);
        float env = (1.0f - expf(-200.0f * t)) * expf(-13.0f * t);
        b[i] = (sinf(ph) * 0.5f + lp * 0.4f) * env;
    }
    Normalize(b, 0.6f);
    return b;
}

static std::vector<float> BuildSharkHit() {
    int n = (int)(kRate * 0.3f);
    std::vector<float> b(n);
    float ph = 0.0f, lp = 0.0f;
    for (int i = 0; i < n; i++) {
        float t = i / (float)kRate;
        float f = 210.0f - 120.0f * (1.0f - expf(-18.0f * t));
        ph += 2.0f * kPi * f / kRate;
        float env = (1.0f - expf(-350.0f * t)) * expf(-9.0f * t);
        float noise = RandF01() * 2.0f - 1.0f;
        lp += 0.2f * (noise - lp);
        b[i] = ((sinf(ph) > 0.0f ? 0.5f : -0.5f) + lp * 0.5f) * env;
    }
    Normalize(b, 0.8f);
    return b;
}

static std::vector<float> BuildBuy() {
    int n = (int)(kRate * 0.5f);
    std::vector<float> b(n);
    for (int i = 0; i < n; i++) {
        float t = i / (float)kRate;
        b[i] = Note(t, 880.00f, 0.00f, 0.35f) * 0.6f
             + Note(t, 1174.66f, 0.10f, 0.40f) * 0.5f
             + Note(t, 1568.00f, 0.20f, 0.40f) * 0.4f;
    }
    Normalize(b, 0.7f);
    return b;
}

static std::vector<float> BuildDenied() {
    int n = (int)(kRate * 0.3f);
    std::vector<float> b(n);
    for (int i = 0; i < n; i++) {
        float t = i / (float)kRate;
        b[i] = SquareNote(t, 165.0f, 0.0f, 0.16f) * 0.6f
             + SquareNote(t, 120.0f, 0.12f, 0.18f) * 0.6f;
    }
    Normalize(b, 0.6f);
    return b;
}

static std::vector<float> BuildDive() {
    int n = (int)(kRate * 0.55f);
    std::vector<float> b(n);
    float ph = 0.0f, lp = 0.0f, lp2 = 0.0f;
    for (int i = 0; i < n; i++) {
        float t = i / (float)kRate;
        float noise = RandF01() * 2.0f - 1.0f;
        float cut = 0.5f * expf(-6.0f * t) + 0.01f;
        lp  += cut * (noise - lp);
        lp2 += 0.1f * (lp - lp2);
        float f = 600.0f - 420.0f * (1.0f - expf(-8.0f * t));       // "bloop"
        ph += 2.0f * kPi * f / kRate;
        float env = (1.0f - expf(-150.0f * t)) * expf(-5.0f * t);
        b[i] = (lp2 * 1.2f + sinf(ph) * 0.3f) * env;
    }
    Normalize(b, 0.8f);
    return b;
}

static std::vector<float> BuildSurface() {
    int n = (int)(kRate * 0.75f);
    std::vector<float> b(n);
    const float notes[4] = {523.25f, 659.25f, 783.99f, 1046.50f};   // C E G C
    for (int i = 0; i < n; i++) {
        float t = i / (float)kRate, v = 0.0f;
        for (int k = 0; k < 4; k++) v += Note(t, notes[k], k * 0.09f, 0.5f) * 0.45f;
        b[i] = v;
    }
    Normalize(b, 0.7f);
    return b;
}

static std::vector<float> BuildDeath() {
    int n = (int)(kRate * 1.1f);
    std::vector<float> b(n);
    float ph = 0.0f;
    for (int i = 0; i < n; i++) {
        float t = i / (float)kRate;
        float f = 440.0f - 350.0f * (1.0f - expf(-2.2f * t));        // long slide down
        f *= 1.0f + 0.02f * sinf(2.0f * kPi * 6.0f * t);
        ph += 2.0f * kPi * f / kRate;
        float saw = 2.0f * (ph / (2.0f * kPi) - floorf(ph / (2.0f * kPi))) - 1.0f;
        float env = (1.0f - expf(-60.0f * t)) * expf(-3.2f * t);
        b[i] = (saw * 0.5f + sinf(ph) * 0.5f) * env;
    }
    Normalize(b, 0.85f);
    return b;
}

static std::vector<float> BuildWin() {
    int n = (int)(kRate * 1.4f);
    std::vector<float> b(n);
    const float notes[5] = {523.25f, 659.25f, 783.99f, 1046.50f, 1318.51f};
    for (int i = 0; i < n; i++) {
        float t = i / (float)kRate, v = 0.0f;
        for (int k = 0; k < 5; k++) v += Note(t, notes[k], k * 0.12f, 0.6f) * 0.4f;
        v += Note(t, 523.25f,  0.6f, 0.8f) * 0.3f;
        v += Note(t, 659.25f,  0.6f, 0.8f) * 0.3f;
        v += Note(t, 783.99f,  0.6f, 0.8f) * 0.3f;
        v += Note(t, 1046.50f, 0.6f, 0.8f) * 0.3f;
        b[i] = v;
    }
    Normalize(b, 0.75f);
    return b;
}

static std::vector<float> BuildLowOxygen() {
    int n = (int)(kRate * 0.34f);
    std::vector<float> b(n);
    for (int i = 0; i < n; i++) {
        float t = i / (float)kRate;
        b[i] = SquareNote(t, 950.0f, 0.00f, 0.10f) * 0.5f
             + SquareNote(t, 760.0f, 0.18f, 0.12f) * 0.5f;
    }
    Normalize(b, 0.55f);
    return b;
}

static std::vector<float> BuildMenu() {
    int n = (int)(kRate * 0.07f);
    std::vector<float> b(n);
    for (int i = 0; i < n; i++)
        b[i] = Note(i / (float)kRate, 1200.0f, 0.0f, 0.06f) * 0.5f;
    Normalize(b, 0.45f);
    return b;
}

static float Midi(float m) { return 440.0f * powf(2.0f, (m - 69.0f) / 12.0f); }

// Soft attack/release envelope so notes fade in and out instead of clicking.
static float VoiceEnv(float lt, float len, float attack, float release) {
    if (lt < 0.0f || lt > len) return 0.0f;
    float a = (lt < attack) ? lt / attack : 1.0f;
    float r = ((len - lt) < release) ? (len - lt) / release : 1.0f;
    return a * r;
}

// A gentle, seamless background tune: a low bass line, sustained chord pads
// and a simple arpeggio melody, in C major over the progression C - Am - F - G.
// Every note finishes before the loop point, so the wave meets cleanly.
static std::vector<float> BuildMusic() {
    const float beat = 0.60f;              // 100 BPM
    const float bar  = beat * 4.0f;        // 2.4 s per bar
    const float dur  = bar * 4.0f;         // 9.6 s loop, four bars
    int n = (int)(kRate * dur);
    std::vector<float> b(n, 0.0f);

    auto addTone = [&](float start, float len, float freq, float amp) {
        int s = (int)(start * kRate), e = (int)((start + len) * kRate);
        if (s < 0) s = 0;
        if (e > n) e = n;
        float ph = 0.0f;
        for (int i = s; i < e; i++) {
            float lt  = i / (float)kRate - start;
            float env = VoiceEnv(lt, len, 0.03f, 0.30f);
            ph += 2.0f * kPi * freq / kRate;
            b[i] += sinf(ph) * amp * env;
        }
    };

    const int bassMidi[4]    = {48, 45, 41, 43};                            // C3 A2 F2 G2
    const int chords[4][3]   = {{60,64,67},{57,60,64},{53,57,60},{55,59,62}};   // C Am F G
    const int melody[4][8] = {
        {72,76,79,76,84,79,76,79},
        {69,72,76,72,81,76,72,76},
        {65,69,72,69,77,72,69,72},
        {67,71,74,71,79,74,71,74},
    };

    for (int barIdx = 0; barIdx < 4; barIdx++) {
        float t0 = barIdx * bar;
        addTone(t0, bar - 0.15f, Midi(bassMidi[barIdx]), 0.40f);            // bass
        for (int k = 0; k < 3; k++)                                          // chord pad
            addTone(t0, bar - 0.20f, Midi(chords[barIdx][k]), 0.09f);
        for (int e = 0; e < 8; e++)                                          // melody
            addTone(t0 + e * beat * 0.5f, beat * 0.5f * 0.92f, Midi(melody[barIdx][e]), 0.20f);
    }

    Normalize(b, 0.55f);
    return b;
}

// A seamless "you are underwater" bed: muffled flow, a low rumble and the
// occasional bubble. The tail is cross-faded into the head so the loop seam
// is inaudible.
static std::vector<float> BuildWater() {
    const float dur = 6.0f;
    const int   N   = (int)(kRate * dur);
    const int   X   = (int)(kRate * 0.5f);     // cross-fade length
    std::vector<float> raw(N + X, 0.0f);

    float lp = 0.0f, lp2 = 0.0f;
    for (int i = 0; i < N + X; i++) {
        float t  = i / (float)kRate;
        float nz = RandF01() * 2.0f - 1.0f;
        lp  += 0.030f * (nz - lp);             // watery hiss
        lp2 += 0.004f * (nz - lp2);            // deep rumble
        float swell = 0.8f + 0.2f * sinf(2.0f * kPi * 0.18f * t);
        raw[i] = (lp * 0.55f + lp2 * 1.6f) * swell;
    }

    for (int q = 0; q < 10; q++) {              // bubbles, kept away from the seam
        int start = (int)((0.3f + RandF01() * (dur - 1.0f)) * kRate);
        int len   = (int)(kRate * 0.1f);
        float f0  = 700.0f + RandF01() * 900.0f;
        for (int i = 0; i < len && start + i < N + X; i++) {
            float t = i / (float)kRate;
            float f = f0 + 1600.0f * (t / 0.1f);
            raw[start + i] += sinf(2.0f * kPi * f * t) * expf(-26.0f * t) * 0.22f;
        }
    }

    std::vector<float> b(raw.begin(), raw.begin() + N);
    for (int i = 0; i < X; i++) {               // blend the tail into the head
        float t = i / (float)X;
        b[N - X + i] = raw[N - X + i] * (1.0f - t) + raw[i] * t;
    }
    Normalize(b, 0.5f);
    return b;
}

// ------------------------- WAV packaging for Music -------------------------

static void AppendWav(std::vector<unsigned char>& out, const std::vector<float>& buf) {
    unsigned int dataSize = (unsigned int)buf.size() * 2;
    auto put32 = [&](unsigned int v) {
        out.push_back((unsigned char)(v & 0xff));
        out.push_back((unsigned char)((v >> 8) & 0xff));
        out.push_back((unsigned char)((v >> 16) & 0xff));
        out.push_back((unsigned char)((v >> 24) & 0xff));
    };
    auto put16 = [&](unsigned short v) {
        out.push_back((unsigned char)(v & 0xff));
        out.push_back((unsigned char)((v >> 8) & 0xff));
    };

    const char* riff = "RIFF"; out.insert(out.end(), riff, riff + 4);
    put32(36 + dataSize);
    const char* wave = "WAVE"; out.insert(out.end(), wave, wave + 4);
    const char* fmt  = "fmt "; out.insert(out.end(), fmt, fmt + 4);
    put32(16);
    put16(1);                       // PCM
    put16(1);                       // mono
    put32(kRate);
    put32(kRate * 2);               // byte rate
    put16(2);                       // block align
    put16(16);                      // bits per sample
    const char* data = "data"; out.insert(out.end(), data, data + 4);
    put32(dataSize);
    for (float f : buf) {
        short s = (short)(Clampf(f, -1.0f, 1.0f) * 32000.0f);
        put16((unsigned short)s);
    }
}

// ------------------------- public API -------------------------

void InitAudio() {
    if (!IsAudioDeviceReady()) InitAudioDevice();
    if (!IsAudioDeviceReady()) { g_ready = false; return; }   // headless / no sound card
    g_ready = true;

    struct { SfxId id; std::vector<float> (*build)(); } table[] = {
        {SFX_COLLECT,    BuildCollect},
        {SFX_BAG_FULL,   BuildBagFull},
        {SFX_HURT,       BuildHurt},
        {SFX_STING,      BuildSting},
        {SFX_EXPLODE,    BuildExplode},
        {SFX_HARPOON,    BuildHarpoon},
        {SFX_SHARK_HIT,  BuildSharkHit},
        {SFX_BUY,        BuildBuy},
        {SFX_DENIED,     BuildDenied},
        {SFX_DIVE,       BuildDive},
        {SFX_SURFACE,    BuildSurface},
        {SFX_DEATH,      BuildDeath},
        {SFX_WIN,        BuildWin},
        {SFX_LOW_OXYGEN, BuildLowOxygen},
        {SFX_MENU,       BuildMenu},
    };
    for (auto& e : table) {
        SfxVoice& v = g_sfx[e.id];
        v.base = MakeSound(e.build());
        for (int i = 0; i < kVoices; i++) v.voices[i] = LoadSoundAlias(v.base);
    }

    AppendWav(g_musicWav, BuildMusic());
    g_music = LoadMusicStreamFromMemory(".wav", g_musicWav.data(), (int)g_musicWav.size());
    g_music.looping = true;
    SetMusicVolume(g_music, g_musicVol);

    AppendWav(g_waterWav, BuildWater());
    g_water = LoadMusicStreamFromMemory(".wav", g_waterWav.data(), (int)g_waterWav.size());
    g_water.looping = true;
    SetMusicVolume(g_water, g_waterVol);

    SetMasterVolume(g_muted ? 0.0f : 0.9f);
}

void CloseAudio() {
    if (!g_ready) return;
    for (int id = 0; id < SFX_COUNT; id++) {
        for (int i = 0; i < kVoices; i++) UnloadSoundAlias(g_sfx[id].voices[i]);
        UnloadSound(g_sfx[id].base);
    }
    UnloadMusicStream(g_music);
    UnloadMusicStream(g_water);
    g_musicWav.clear();
    g_waterWav.clear();
    CloseAudioDevice();
    g_ready = false;
}

bool AudioReady() { return g_ready; }

void PlaySfx(SfxId id, float pitch) {
    if (!g_ready || g_muted) return;
    if (id < 0 || id >= SFX_COUNT) return;
    SfxVoice& v = g_sfx[id];
    Sound& s = v.voices[v.next];
    v.next = (v.next + 1) % kVoices;
    SetSoundVolume(s, g_sfxVol);
    SetSoundPitch(s, pitch);
    PlaySound(s);
}

void UpdateAudio() {
    if (!g_ready) return;
    if (IsMusicStreamPlaying(g_music)) UpdateMusicStream(g_music);
    if (IsMusicStreamPlaying(g_water)) UpdateMusicStream(g_water);
}

void StartMusic() {
    if (!g_ready) return;
    PlayMusicStream(g_music);
}

void StopMusic() {
    if (!g_ready) return;
    StopMusicStream(g_music);
}

void StartWater() {
    if (!g_ready) return;
    PlayMusicStream(g_water);
}

void StopWater() {
    if (!g_ready) return;
    StopMusicStream(g_water);
}

void SetAudioMuted(bool muted) {
    g_muted = muted;
    if (g_ready) SetMasterVolume(muted ? 0.0f : 0.9f);
}

bool IsAudioMuted() { return g_muted; }

void ToggleAudioMuted() { SetAudioMuted(!g_muted); }

void SetSfxVolume(float volume) {
    g_sfxVol = Clampf(volume, 0.0f, 1.0f);
}

float GetSfxVolume() { return g_sfxVol; }

void SetBgmVolume(float volume) {
    g_musicVol = Clampf(volume, 0.0f, 1.0f);
    if (g_ready) SetMusicVolume(g_music, g_musicVol);
}

float GetBgmVolume() { return g_musicVol; }

void SetWaterVolume(float volume) {
    g_waterVol = Clampf(volume, 0.0f, 1.0f);
    if (g_ready) SetMusicVolume(g_water, g_waterVol);
}

float GetWaterVolume() { return g_waterVol; }
