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
static Music    g_ambient{};
static std::vector<unsigned char> g_ambientWav;   // kept alive while the stream reads it
static bool g_ready  = false;
static bool g_muted  = false;

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

// An 8 second seamless loop: a low drone plus a band of soft partials
// that reads as muffled ocean noise. Every frequency is an exact multiple
// of 1/duration so the waveform meets cleanly when it repeats.
static std::vector<float> BuildAmbient() {
    const float dur = 8.0f;
    int n = (int)(kRate * dur);
    std::vector<float> b(n, 0.0f);

    auto addTone = [&](float freq, float amp) {
        int k = (int)floorf(freq * dur + 0.5f);
        if (k < 1) k = 1;
        float f = k / dur;                       // snap to a looping multiple
        float ph = RandF01() * 2.0f * kPi;
        for (int i = 0; i < n; i++) { ph += 2.0f * kPi * f / kRate; b[i] += sinf(ph) * amp; }
    };

    const float drone[5]  = {55.0f, 82.5f, 110.0f, 165.0f, 220.0f};
    const float dAmp[5]   = {0.50f, 0.32f, 0.22f, 0.14f, 0.08f};
    for (int i = 0; i < 5; i++) addTone(drone[i], dAmp[i]);

    for (int p = 0; p < 48; p++) {               // soft "water" texture
        float freq = 20.0f + RandF01() * 3200.0f;
        float amp  = 0.35f / sqrtf(fmaxf(freq, 1.0f));
        addTone(freq, amp);
    }

    float lfoPh = 0.0f;
    float lfoF  = 0.25f;                         // 2 cycles over the loop
    for (int i = 0; i < n; i++) {
        lfoPh += 2.0f * kPi * lfoF / kRate;
        b[i] *= 0.85f + 0.15f * sinf(lfoPh);
    }

    for (int q = 0; q < 6; q++) {                // occasional bubbles
        int start = (int)((0.5f + RandF01() * (dur - 1.5f)) * kRate);
        int len   = (int)(kRate * 0.12f);
        for (int i = 0; i < len && start + i < n; i++) {
            float t = i / (float)kRate;
            float f = 900.0f + 1400.0f * t / 0.12f;   // rising pop
            b[start + i] += sinf(2.0f * kPi * f * t) * expf(-22.0f * t) * 0.25f;
        }
    }

    Normalize(b, 0.55f);
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

    AppendWav(g_ambientWav, BuildAmbient());
    g_ambient = LoadMusicStreamFromMemory(".wav", g_ambientWav.data(), (int)g_ambientWav.size());
    g_ambient.looping = true;
    SetMusicVolume(g_ambient, 0.35f);

    SetMasterVolume(g_muted ? 0.0f : 0.9f);
}

void CloseAudio() {
    if (!g_ready) return;
    for (int id = 0; id < SFX_COUNT; id++) {
        for (int i = 0; i < kVoices; i++) UnloadSoundAlias(g_sfx[id].voices[i]);
        UnloadSound(g_sfx[id].base);
    }
    UnloadMusicStream(g_ambient);
    g_ambientWav.clear();
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
    SetSoundPitch(s, pitch);
    PlaySound(s);
}

void UpdateAudio() {
    if (g_ready && IsMusicStreamPlaying(g_ambient)) UpdateMusicStream(g_ambient);
}

void StartAmbient() {
    if (!g_ready) return;
    PlayMusicStream(g_ambient);
}

void StopAmbient() {
    if (!g_ready) return;
    StopMusicStream(g_ambient);
}

void SetAudioMuted(bool muted) {
    g_muted = muted;
    if (g_ready) SetMasterVolume(muted ? 0.0f : 0.9f);
}

bool IsAudioMuted() { return g_muted; }

void ToggleAudioMuted() { SetAudioMuted(!g_muted); }
