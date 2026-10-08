#include "Helpers.h"
#include <cstdlib>
#include <algorithm>
#include "Globals.h"
#include "Constants.h"

float RandF(float a, float b) { return a + (b - a) * (rand() / (float)RAND_MAX); }
float Clamp01(float v)        { return std::max(0.0f, std::min(1.0f, v)); }
float DepthM(float y)         { return std::max(0.0f, y / PX_PER_METER); }

int BagCapacity() { return START_BAG + 3 * save.level[UP_BAG]; }

int BagCount() {
    int n = 0;
    for (int i = 0; i < 5; i++) n += bag[i];
    return n;
}

int BagValue() {
    int v = 0;
    for (int i = 0; i < 5; i++) v += bag[i] * TYPES[i].value;
    return v;
}

int UpgradeCost(int i) { return UPGRADES[i].baseCost * (save.level[i] + 1); }

Color LerpColor(Color a, Color b, float t) {
    return Color{(unsigned char)(a.r + (b.r - a.r) * t),
                 (unsigned char)(a.g + (b.g - a.g) * t),
                 (unsigned char)(a.b + (b.b - a.b) * t), 255};
}

Color WaterColor(float depthM) {
    // Ocean gradient: sunlit turquoise -> vivid blue -> deep navy.
    float f = Clamp01(depthM / MAX_DEPTH_M);
    const Color surface = Color{52, 190, 235, 255};
    const Color mid     = Color{14, 96, 200, 255};
    const Color deep    = Color{2, 16, 58, 255};
    if (f < 0.5f) return LerpColor(surface, mid, f / 0.5f);
    return LerpColor(mid, deep, (f - 0.5f) / 0.5f);
}

void ShowMessage(const std::string& s, float secs) { message = s; messageTimer = secs; }

const char* LayerName(float d) {
    if (d < 100) return "Layer 1: Sunlit Shallows";
    if (d < 250) return "Layer 2: Shark Waters";
    if (d < 500) return "Layer 3: Currents & Mines";
    return "Layer 4: The Abyss";
}

// How far (in pixels) the diver can see. Shrinks with depth, grows with flashlight.
float VisionRadius(float depthM) {
    float f = Clamp01(depthM / MAX_DEPTH_M);
    return 900.0f - 780.0f * f + 45.0f * save.level[UP_LIGHT];
}

bool CanSee(Vector2 p) {
    float dx = p.x - diver.pos.x, dy = p.y - diver.pos.y;
    float r  = VisionRadius(DepthM(diver.pos.y));
    return dx * dx + dy * dy < r * r;
}