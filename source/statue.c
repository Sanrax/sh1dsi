// SPDX-License-Identifier: GPL-3.0-only
// Adapted from MAP1_S01 event 6 and the original smoke parameters.
// Copyright (C) 2026 shdecompilations
#include <nds.h>
#include "statue.h"
#include "player.h"
#include "pickup.h"

enum { WarpAge = 67, SmokeAge = 219, FadeAge = 581, FadeLength = 29 };

static struct {
    uint32_t bytes;
    uint16_t loop, rate;
    uint8_t volume, pan;
    uint16_t reserved;
} audio;
_Static_assert(sizeof audio == 12, "statue sound layout");
static int16_t samples[40*1024] __attribute__((aligned(32)));
static unsigned smokeTexture, smokePalette;
static int channel = -1;

typedef struct {
    int16_t x, z;
    uint8_t offset, period, tile;
} Particle;
static Particle particles[150];
static unsigned age, fadeStart;
static bool closeCamera, fumes, opened;

bool statueLoad(FILE *file, unsigned textureOffset, unsigned palette)
{
    if (fread(&audio, sizeof audio, 1, file) != 1 || !audio.bytes ||
        audio.bytes > sizeof samples || !audio.rate || (audio.bytes & 3) ||
        (audio.loop & 1) || 2u*audio.loop >= audio.bytes || audio.volume > 127 || audio.pan > 127 ||
        audio.reserved)
        return false;
    // Keep the sample in the room stream's read-ahead window.
    for (unsigned offset = 0; offset < audio.bytes;) {
        unsigned bytes = audio.bytes-offset;
        if (bytes > 2048) bytes = 2048;
        if (fread((uint8_t *)samples + offset, 1, bytes, file) != bytes)
            return false;
        offset += bytes;
    }
    DC_FlushRange(samples, audio.bytes);
    smokeTexture = textureOffset >> 3;
    smokePalette = palette*2;
    return true;
}

bool statueActive(void) { return age != 0; }
bool statueCamera(void) { return closeCamera; }
bool statueOpened(void) { return opened; }
unsigned statueFlags(void) { return opened ? HandOpened : 0; }
void statueRestore(unsigned flags) { opened = (flags & HandOpened) != 0; }

bool statueStart(void)
{
    if (age || opened || !roomCanDissolveHand() || !playerFacingHand() ||
        !(pickupFlags() & ChemicalTaken)) return false;
    age = 1;
    fadeStart = 0;
    fumes = false;
    return true;
}

static void startSmoke(void)
{
    uint32_t random = 1;
    for (unsigned i = 0; i < sizeof particles/sizeof particles[0]; i++) {
        random = random*1664525u+1013904223u;
        unsigned angle = (random >> 16) & 32767, radius = (random >> 4) % 369;
        unsigned period = 65+(random >> 26)%22;
        particles[i] = (Particle){(int)radius*cosLerp(angle)/4096, (int)radius*sinLerp(angle)/4096,
                                  i*period/(sizeof particles/sizeof particles[0]), period, (random >> 24)%3};
    }
    // The repeat point is in words. The length register covers the loop only.
    channel = soundPlaySample(samples, SoundFormat_16Bit, audio.bytes-2u*audio.loop, audio.rate,
                              audio.volume, audio.pan, true, audio.loop/2);
}

bool statueTick(unsigned pressed)
{
    if (!age) return false;
    ++age;
    if (!fadeStart && (age == FadeAge || (pressed & KEY_START))) {
        fadeStart = age;
        if (channel >= 0) soundKill(channel);
        channel = -1;
    }
    if (fadeStart) {
        unsigned fade = age-fadeStart;
        if (fade >= FadeLength) {
            setBrightness(1, -16);
            opened = true;
            closeCamera = false;
            age = 0;
            return true;
        }
        setBrightness(1, -(int)(16*fade/FadeLength));
    } else if (age == WarpAge) {
        playerWarpForPour();
        closeCamera = true;
    } else if (age == SmokeAge) {
        startSmoke();
        fumes = true;
    }
    return false;
}

unsigned statueBorder(void)
{
    if (closeCamera) return 19;
    unsigned slide = fadeStart ? fadeStart : age;
    return 6+13*slide/WarpAge;
}

void statueDrawSmoke(void)
{
    if (!fumes || age < SmokeAge || age >= FadeAge) return;
    unsigned elapsed = age-SmokeAge;
    GFX_TEX_FORMAT = smokeTexture | (TEXTURE_SIZE_32 << 20) | (TEXTURE_SIZE_128 << 23) | (GL_RGB32_A3 << 26);
    GFX_PAL_FORMAT = smokePalette;
    glColor(RGB15(31,31,31));
    for (unsigned i = 0; i < sizeof particles/sizeof particles[0]; i++) {
        const Particle *p = &particles[i];
        unsigned t = elapsed+p->offset;
        // The original sustains the cloud for three seconds, then lets it dissipate.
        if (elapsed >= 180 && t/p->period != (180+p->offset)/p->period) continue;
        t %= p->period;
        unsigned alpha = 13*(p->period-t)/p->period;
        if (!alpha) continue;
        int half = t < 10 ? 128+13*t : 256;
        // Billboard basis for the authored cutscene camera in DS world axes.
        int rx = (half*3930)>>12, rz = (half*1171)>>12;
        int ux = -(half*549)>>12, uy = (half*3610)>>12, uz = (half*1840)>>12;
        int x = (3277+p->x*(int)(128+t)/128)/2;
        int z = (2048+p->z*(int)(128+t)/128)/2;
        int y = 2048+14*t, v = p->tile*32;
        glPolyFmt(POLY_ALPHA(alpha) | POLY_ID(1+i%62) | POLY_CULL_NONE);
        glBegin(GL_QUADS);
        glTexCoord2t16(0, v<<4);
        glVertex3v16(x-rx+ux, y+uy, z-rz+uz);
        glTexCoord2t16(31<<4, v<<4);
        glVertex3v16(x+rx+ux, y+uy, z+rz+uz);
        glTexCoord2t16(31<<4, (v+31)<<4);
        glVertex3v16(x+rx-ux, y-uy, z+rz-uz);
        glTexCoord2t16(0, (v+31)<<4);
        glVertex3v16(x-rx-ux, y-uy, z-rz-uz);
        glEnd();
    }
}
