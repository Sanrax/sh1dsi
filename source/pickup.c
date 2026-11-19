// SPDX-License-Identifier: GPL-3.0-only
// Adapted from MAP1_S01 event 11 and Gfx_PickupItemAnimate.
// Copyright (C) 2026 shdecompilations
#include <nds.h>
#include <string.h>
#include "pickup.h"

typedef struct {
    int16_t x, y, z;
    uint8_t u, v;
    uint32_t normal;
} Corner;

typedef struct {
    uint8_t x, y, ink, reserved, mask[14][11];
} Glyph;

static Corner corners[156*3];
static Glyph glyphs[64];
static uint16_t glyphCount, messageLength, fontPalette[16];
static unsigned textureAddress, paletteAddress;
static enum { Inactive, Stopping, Kneeling, Inspecting, Rising } phase;
static unsigned ticks, selected, flash, shown;
static struct { uint8_t id, count; } inventoryItem;

static Glyph latchGlyphs[24];
static uint16_t latchCount;
static bool unlocked;
static unsigned latchAge, latchShown;

static bool readGlyphs(FILE *file, Glyph *data, unsigned count)
{
    if (fread(data, sizeof *data, count, file) != count) return false;
    for (unsigned i = 0; i < count; i++) {
        const Glyph *g = &data[i];
        if (g->x > 256-11 || g->y > 192-14 || g->ink > 3) return false;
        for (unsigned y = 0; y < 14; y++)
            for (unsigned x = 0; x < 11; x++)
                if (g->mask[y][x] > 15) return false;
    }
    return true;
}

bool pickupLoad(FILE *file, unsigned textureOffset, unsigned palette)
{
    textureAddress = textureOffset >> 3;
    paletteAddress = palette * 2;
    if (fread(corners, sizeof corners, 1, file) != 1 ||
        fread(&glyphCount, 2, 1, file) != 1 || glyphCount > 64 ||
        fread(&messageLength, 2, 1, file) != 1 || !messageLength || messageLength >= glyphCount ||
        fread(fontPalette, sizeof fontPalette, 1, file) != 1 ||
        !readGlyphs(file, glyphs, glyphCount))
        return false;
    return true;
}

bool latchLoad(FILE *file)
{
    uint16_t count, length, palette[16];
    if (fread(&count, 2, 1, file) != 1 || !count || count > 24 ||
        fread(&length, 2, 1, file) != 1 || count != length ||
        fread(palette, sizeof palette, 1, file) != 1 ||
        memcmp(palette, fontPalette, sizeof palette) || !readGlyphs(file, latchGlyphs, count))
        return false;
    latchCount = count;
    return true;
}

bool latchStart(void)
{
    if (unlocked || !latchCount || latchAge) return false;
    latchAge = 1;
    latchShown = 0;
    return true;
}

bool latchUnlocked(void) { return unlocked; }

bool latchActive(void) { return latchAge != 0; }

void latchTick(unsigned pressed)
{
    if (!latchAge) return;
    // Event_DisplayMapMsgWithSfx waits 0.2 seconds before starting the text.
    if (++latchAge <= 12) return;
    if (latchShown < latchCount) {
        latchShown = (pressed & (KEY_A | KEY_B)) ? latchCount : latchShown+1;
    } else if (pressed & (KEY_A | KEY_B)) {
        unlocked = true;
        latchAge = latchShown = 0;
    }
}

static bool hasSilverMedallion(void)
{
    return inventoryItem.id == 98 && inventoryItem.count != 0;
}

bool pickupActive(void)
{
    return phase != Inactive;
}

void pickupStart(void)
{
    if (!pickupActive() && !hasSilverMedallion() && playerFacingMedallion())
        phase = Stopping;
}

PlayerAction pickupAction(void)
{
    if (phase == Kneeling || phase == Inspecting) return PlayerKneel;
    return phase == Rising ? PlayerRise : PlayerMove;
}

bool pickupFloorHidden(void)
{
    return phase == Inspecting || hasSilverMedallion();
}

void pickupTick(unsigned pressed)
{
    switch (phase) {
    case Stopping:
        if (playerStopped()) phase = Kneeling;
        break;
    case Kneeling:
        if (playerActionDone(PlayerKneel)) {
            ticks = selected = flash = shown = 0;
            phase = Inspecting;
        }
        break;
    case Inspecting:
        ticks++;
        if (ticks < 16) break;
        if (shown < glyphCount) {
            shown = (pressed & (KEY_A|KEY_B)) ? glyphCount : ticks-16;
            if (shown >= messageLength) shown = glyphCount;
        } else {
            flash++;
            unsigned previous = selected;
            if (pressed & KEY_UP) selected = 0;
            if (pressed & KEY_DOWN) selected = 1;
            if (previous != selected) flash = 0;
            if (pressed & (KEY_A|KEY_B)) {
                if ((pressed & KEY_A) && !selected && !(pressed & KEY_B)) {
                    inventoryItem.id = 98;
                    inventoryItem.count = 1;
                }
                phase = Rising;
            }
        }
        break;
    case Rising:
        if (playerActionDone(PlayerRise)) phase = Inactive;
        break;
    default:
        break;
    }
}

void pickupDrawItem(void)
{
    if (phase != Inspecting || ticks < 4) return;
    // Keep the room's perspective distance for the pickup in its own view.
    // Place its depth interval in front of the gameplay geometry.
    const int depth = 1872*4, near = 1450*4;
    glViewport(0, 0, 255, 191);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustumf32(-160*near/224, 160*near/224, -120*near/224, 120*near/224, near, 64*4096);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef32(0, -64*4, -depth);
    glLight(0, RGB15(31,31,31), 0, 0, -511);
    glMaterialf(GL_AMBIENT, RGB15(16,16,16));
    glMaterialf(GL_DIFFUSE, RGB15(26,26,26));
    unsigned age = ticks & ~1u;
    unsigned angle = age > 18 ? ((age-18)%120)*32768u/120 : 0;
    glRotateYi(-(int)(angle & 32767));
    glRotateXi(4096);
    int scale = age < 18 ? (int)(age-2)*4096/15 : 4096;
    glScalef32(scale, scale, scale);
    GFX_TEX_FORMAT = textureAddress | (TEXTURE_SIZE_64 << 20) | (TEXTURE_SIZE_64 << 23) | (GL_RGB16 << 26);
    GFX_PAL_FORMAT = paletteAddress;
    glPolyFmt(POLY_ALPHA(31) | POLY_CULL_FRONT | POLY_FORMAT_LIGHT0);
    glBegin(GL_TRIANGLES);
    for (unsigned i = 0; i < sizeof corners / sizeof corners[0]; i++) {
        const Corner *c = &corners[i];
        glNormal(c->normal);
        glTexCoord2t16(c->u << 4, c->v << 4);
        glVertex3v16(c->x*4, c->y*4, c->z*4);
    }
    glEnd();
}

void pickupDrawPrompt(int bg)
{
    static unsigned lastShown, lastHighlight;
    unsigned visible = latchAge ? latchShown : phase == Inspecting ? shown : 0;
    const Glyph *text = latchAge ? latchGlyphs : glyphs;
    if (!visible) {
        bgHide(bg);
        lastShown = 0;
        return;
    }
    uint16_t *pixels = bgGetGfxPtr(bg);
    if (!lastShown)
        dmaFillHalfWords(0, pixels, 256*192);
    for (unsigned i = lastShown; i < visible; i++) {
        const Glyph *g = &text[i];
        for (unsigned y = 0; y < 14; y++) {
            uint16_t *row = pixels+(g->y+y)*128;
            for (unsigned x = 0; x < 11; x++) {
                unsigned ink = g->mask[y][x];
                if (!ink) continue;
                // Bitmap VRAM accepts halfword stores. Preserve the adjacent
                // pixel when glyphs share a halfword at their bearings.
                unsigned col = g->x+x, shift = (col & 1)*8;
                row[col/2] = (row[col/2] & ~(255u << shift)) | ((ink+16*g->ink) << shift);
            }
        }
    }
    unsigned highlight = (flash/15)%2 == 0 ? selected : 2;
    if (!lastShown || lastHighlight != highlight) {
        for (unsigned i = 1; i < 16; i++) {
            uint16_t white = fontPalette[i] & 0x7fff;
            unsigned level = white & 31;
            BG_PALETTE[i] = white;
            BG_PALETTE[16+i] = RGB15(level*24/128, level, level*40/128);
            for (unsigned j = 0; j < 2; j++)
                BG_PALETTE[32+j*16+i] = highlight == j ? RGB15(level,0,0) : white;
        }
        lastHighlight = highlight;
    }
    lastShown = visible;
    bgShow(bg);
}
