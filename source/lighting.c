// SPDX-License-Identifier: GPL-3.0-only
// Portions derived from WorldEnv_Update and func_80057658.
// Copyright (C) 2026 shdecompilations
#include <string.h>
#include <nds.h>
#include "lighting.h"

enum { FullBeam = 15 };
static uint32_t projection[512];
static uint16_t shading[256];
static int beam = FullBeam, previousBeam = -1;
static bool enabled = true;
static int intensity, ambient, shadedBeam;
static Flashlight shadedLamp;

void lightingLoadRoom(bool newMap)
{
    if (newMap) {
        beam = FullBeam;
        enabled = true;
    }
    previousBeam = shadedBeam = -1;
    if (!projection[8]) {
        // H=16 in the PS1 lighting projection. The divide caps at 2.0.
        for (unsigned i = 0; i < 512; i++)
            projection[i] = i <= 8 ? 0x1ffff : (16u << 16)/i;
    }
}

bool lightingTick(bool toggle)
{
    if (toggle) enabled = !enabled;
    int target = enabled ? FullBeam : 0;
    if (beam == target) return false;
    beam += enabled ? 1 : -1;
    return true;
}

void lightingEnableFlashlight(void)
{
    enabled = true;
}

void lightingScreen(bool active, int border)
{
    static int backdrop = -1, previousBorder = -1;
    if (!active) {
        if (previousBorder >= 0) {
            bgHide(backdrop);
            bgSetPriority(0, 1);
            REG_BLDCNT = 0;
            previousBorder = -1;
        }
        return;
    }
    if (border == previousBorder) return;
    if (backdrop < 0)
        backdrop = bgInitHidden(2, BgType_Bmp8, BgSize_B8_256x256, 4, 0);
    uint16_t *mask = bgGetGfxPtr(backdrop);
    dmaFillHalfWords(0, mask, border*256);
    dmaFillHalfWords(0xffff, mask+border*128, (192-2*border)*256);
    dmaFillHalfWords(0, mask+(192-border)*128, border*256);
    // The original brightness pass adds three RGB555 steps in the viewport.
    BG_PALETTE[255] = RGB15(3, 3, 3);
    bgSetPriority(0, 2);
    bgSetPriority(backdrop, 1);
    REG_BLDALPHA = 16 | (16 << 8);
    REG_BLDCNT = BLEND_ALPHA | BLEND_SRC_BG2 | BLEND_DST_BG0;
    bgShow(backdrop);
    previousBorder = border;
}

unsigned lightingFog(void)
{
    if (beam != previousBeam) {
        // MAP1_S02's applied endpoint presets.
        intensity = (2867*beam)/FullBeam;
        ambient = (56*(FullBeam-beam))/FullBeam;
        int r = 51+(115-51)*beam/FullBeam;
        int g = 89+(128-89)*beam/FullBeam;
        int b = 128+(134-128)*beam/FullBeam;
        for (int i = 0; i < 256; i++) {
            int red = (r*i+256) >> 9, green = (g*i+256) >> 9, blue = (b*i+256) >> 9;
            shading[i] = RGB15(red < 31 ? red : 31, green < 31 ? green : 31, blue < 31 ? blue : 31);
        }
        if (beam < FullBeam) {
            static const uint8_t curve[] = {
                0, 0, 38, 76, 94, 112, 133, 153, 176, 189, 209, 220, 230, 240, 245, 250, 255
            };
            int lateFade = beam*2-FullBeam;
            int nearDistance = 2380;
            if (lateFade > 0) nearDistance += (10240-2380)*lateFade/FullBeam;
            glFogColor(0,0,0,31);
            glFogShift(8);
            glFogOffset(0);
            // W-buffer fog entries are spaced one world metre apart.
            for (int i = 0; i < 32; i++) {
                unsigned position = (unsigned)(i+1)*256*4096/nearDistance;
                unsigned index = position >> 8;
                unsigned fraction = position & 255;
                glFogDensity(i, index >= 16 ? 127 :
                    (((256-fraction)*curve[index]+fraction*curve[index+1]) >> 9));
            }
        }
        previousBeam = beam;
    }
    if (beam == FullBeam) {
        glDisable(GL_FOG);
        return 0;
    }
    glEnable(GL_FOG);
    return POLY_FOG;
}

uint16_t lightingActorColor(void)
{
    // Harry's current mesh stream shares one tint across the actor.
    return shading[ambient + 65*beam/FullBeam];
}

static inline int screenCue(int dot, unsigned perspective)
{
    if (dot <= 0) return 0;
    int cue = (dot * (int)perspective) >> 16;
    return cue < 2047 ? cue : 2047;
}

static inline uint16_t vertexColor(const LitPoint *v, const Flashlight *lamp,
                                   unsigned positionShift)
{
    int x = (v->x >> positionShift)-lamp->x;
    int y = -(v->y >> positionShift)-lamp->y;
    int z = -(v->z >> positionShift)-lamp->z;
    unsigned depth = (x*x + y*y + z*z) >> 12;
    unsigned shift = depth >= 512 ? 23-__builtin_clz(depth) : 0;
    unsigned perspective = projection[depth >> shift] >> shift;
    int along = screenCue((lamp->dx*x + lamp->dy*y + lamp->dz*z) >> 12, perspective);
    int facing = screenCue((v->nx*x + v->ny*y + v->nz*z) >> 7, perspective);
    int depthCue = (perspective * (intensity >> 8)) >> 12;
    int highlight = ((facing*(along+(depthCue >> 4))) >> 5)-16;
    int spill = (along >> 1)+(depthCue >> 2);
    int light = ambient + (highlight > 0 ? highlight : 0) + (spill < 48 ? spill : 48);
    return shading[light < 255 ? light : 255];
}

ARM_CODE void lightingShade(const LitPoint *points, uint16_t *colors, unsigned count,
                            const Flashlight *lamp, unsigned positionShift)
{
    if (shadedBeam == beam && (!intensity || !memcmp(&shadedLamp, lamp, sizeof *lamp)))
        return;
    shadedBeam = beam;
    shadedLamp = *lamp;
    if (intensity) {
        for (unsigned i = 0; i < count; i++) colors[i] = vertexColor(&points[i], lamp, positionShift);
    } else {
        uint16_t color = shading[ambient];
        for (unsigned i = 0; i < count; i++) colors[i] = color;
    }
}
