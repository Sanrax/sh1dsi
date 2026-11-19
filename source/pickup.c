// SPDX-License-Identifier: GPL-3.0-only
// Adapted from MAP1_S01 event 11, MAP1_S00 CommonItemTake, Event_ItemTake
// and Gfx_PickupItemAnimate.
// Copyright (C) 2026 shdecompilations
#include <nds.h>
#include <string.h>
#include "pickup.h"
#include "glyph.h"

typedef struct {
    int16_t x, y, z;
    uint8_t u, v;
    uint32_t normal;
} Corner;

typedef struct {
    bool loaded;
    Glyph glyphs[64];
    uint16_t glyphCount, messageLength;
    unsigned textureAddress, paletteAddress;
} Model;

static Corner silverCorners[156*3], drinkCorners[124*3], kitCorners[76*3];
static const struct {
    int x, z;
    unsigned flag;
    Corner *corners;
    size_t cornerCount;
} pickups[] = {
    [SilverMedallion] = {-404684, 93388, SilverHeld, silverCorners, sizeof silverCorners/sizeof(Corner)},
    [HealthDrink] = {250961, 586956, DrinkHeld, drinkCorners, sizeof drinkCorners/sizeof(Corner)},
    [FirstAidKit] = {247971, 583680, KitHeld, kitCorners, sizeof kitCorners/sizeof(Corner)},
};
static Model models[sizeof pickups/sizeof pickups[0]];
static uint16_t fontPalette[16];
static enum { Inactive, Stopping, Kneeling, Preparing, Inspecting, Rising } phase;
static PickupItem activeItem;
static unsigned ticks, selected, flash, shown;
static unsigned collected;

static Glyph latchGlyphs[24];
static uint16_t latchCount;
static bool unlocked;
static unsigned latchAge, latchShown;

bool pickupLoad(FILE *file, PickupItem item, unsigned textureOffset, unsigned palette)
{
    Model *m = &models[item];
    unsigned count = pickups[item].cornerCount;
    if (fread(pickups[item].corners, sizeof(Corner), count, file) != count ||
        fread(&m->glyphCount, 2, 1, file) != 1 || m->glyphCount > 64 ||
        fread(&m->messageLength, 2, 1, file) != 1 ||
        !m->messageLength || m->messageLength >= m->glyphCount ||
        fread(fontPalette, sizeof fontPalette, 1, file) != 1 ||
        !glyphsRead(file, m->glyphs, m->glyphCount))
        return false;
    m->loaded = true;
    m->textureAddress = textureOffset >> 3;
    m->paletteAddress = palette * 2;
    return true;
}

bool latchLoad(FILE *file)
{
    uint16_t count, length, palette[16];
    if (fread(&count, 2, 1, file) != 1 || !count || count > 24 ||
        fread(&length, 2, 1, file) != 1 || count != length ||
        fread(palette, sizeof palette, 1, file) != 1 ||
        memcmp(palette, fontPalette, sizeof palette) || !glyphsRead(file, latchGlyphs, count))
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

unsigned pickupFlags(void)
{
    return collected;
}

void pickupRestore(unsigned flags)
{
    collected = flags & PickupFlags;
    unlocked = (flags & LatchOpen) != 0;
}

bool pickupActive(void)
{
    return phase != Inactive;
}

void pickupStart(void)
{
    if (pickupActive()) return;
    PickupItem first, last;
    if (roomCurrent() == MusicRoom) {
        first = last = SilverMedallion;
    } else if (roomCurrent() == Infirmary) {
        first = HealthDrink;
        last = FirstAidKit;
    } else {
        return;
    }
    // MAP1_S00 points 97 and 98 follow their original event scan order.
    for (PickupItem item = first; item <= last; item++) {
        if (!models[item].loaded || pickupHidden(item) ||
            !playerFacingPoint(pickups[item].x, pickups[item].z)) continue;
        activeItem = item;
        ticks = selected = flash = shown = 0;
        phase = item == SilverMedallion ? Stopping : Preparing;
        break;
    }
}

PlayerAction pickupAction(void)
{
    if (activeItem == SilverMedallion && (phase == Kneeling || phase == Inspecting)) return PlayerKneel;
    return phase == Rising ? PlayerRise : PlayerMove;
}

bool pickupHidden(PickupItem item)
{
    return (phase == Inspecting && activeItem == item) || (collected & pickups[item].flag);
}

PickupSound pickupTick(unsigned pressed)
{
    bool silver = activeItem == SilverMedallion;
    const Model *m = &models[activeItem];
    const unsigned promptDelay = silver ? 16 : 21;
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
    case Preparing:
        // The PS1 loads the item model with player control frozen.
        if (++ticks >= (activeItem == HealthDrink ? 23u : 27u)) {
            ticks = 0;
            phase = Inspecting;
        }
        break;
    case Inspecting:
        ticks++;
        if (ticks < promptDelay) break;
        if (shown < m->glyphCount) {
            shown = (pressed & (KEY_A|KEY_B)) ? m->glyphCount : ticks-promptDelay;
            if (shown >= m->messageLength) shown = m->glyphCount;
        } else {
            flash++;
            unsigned previous = selected;
            if (pressed & KEY_UP) selected = 0;
            if (pressed & KEY_DOWN) selected = 1;
            if (previous != selected) flash = 0;
            if (pressed & (KEY_A|KEY_B)) {
                bool accept = (pressed & KEY_A) && !selected && !(pressed & KEY_B);
                if (accept)
                    collected |= pickups[activeItem].flag;
                phase = silver ? Rising : Inactive;
                return accept ? PickupConfirm : PickupCancel;
            }
            if (previous != selected) return PickupMove;
        }
        break;
    case Rising:
        if (playerActionDone(PlayerRise)) phase = Inactive;
        break;
    default:
        break;
    }
    return PickupQuiet;
}

void pickupDrawItem(void)
{
    bool silver = activeItem == SilverMedallion;
    if (phase != Inspecting || ticks < (silver ? 4u : 3u)) return;
    const Model *m = &models[activeItem];
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
    unsigned expanded = silver ? 18 : 21;
    unsigned angle = age > expanded ? ((age-expanded)%120)*32768u/120 : 0;
    glRotateYi(-(int)(angle & 32767));
    glRotateXi(4096);
    int scale = 4096;
    if (age < expanded)
        scale = silver ? (int)(age-2)*4096/15 : (int)age*4096/15;
    if (scale > 4096) scale = 4096;
    glScalef32(scale, scale, scale);
    unsigned height = silver ? TEXTURE_SIZE_64 : TEXTURE_SIZE_256;
    GFX_TEX_FORMAT = m->textureAddress | (TEXTURE_SIZE_64 << 20) | (height << 23) | (GL_RGB16 << 26);
    GFX_PAL_FORMAT = m->paletteAddress;
    glPolyFmt(POLY_ALPHA(31) | POLY_CULL_FRONT | POLY_FORMAT_LIGHT0);
    glBegin(GL_TRIANGLES);
    for (unsigned i = 0; i < pickups[activeItem].cornerCount; i++) {
        const Corner *c = &pickups[activeItem].corners[i];
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
    const Glyph *text = latchAge ? latchGlyphs : models[activeItem].glyphs;
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
