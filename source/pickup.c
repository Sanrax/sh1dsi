// SPDX-License-Identifier: GPL-3.0-only
// Adapted from MAP1_S01 events 5 and 11, MAP1_S00 CommonItemTake, Event_ItemTake
// and Gfx_PickupItemAnimate.
// Copyright (C) 2026 shdecompilations
#include <nds.h>
#include <string.h>
#include "pickup.h"
#include "glyph.h"
#include "statue.h"

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
static Corner chemicalCorners[188*3], goldCorners[156*3];
static Corner lobbyKitCorners[76*3], lobbyBulletCorners[44*3];
static const struct {
    int x, z;
    unsigned flag;
    Corner *corners;
    size_t cornerCount;
} pickups[] = {
    [SilverMedallion] = {-404684, 93388, SilverHeld, silverCorners, sizeof silverCorners/sizeof(Corner)},
    [HealthDrink] = {250961, 586956, DrinkHeld, drinkCorners, sizeof drinkCorners/sizeof(Corner)},
    [FirstAidKit] = {247971, 583680, KitHeld, kitCorners, sizeof kitCorners/sizeof(Corner)},
    [Chemical] = {0, 0, ChemicalTaken, chemicalCorners, sizeof chemicalCorners/sizeof(Corner)},
    [GoldMedallion] = {0, 0, GoldHeld, goldCorners, sizeof goldCorners/sizeof(Corner)},
    [LobbyKit] = {402227, 573849, LobbyKitHeld, lobbyKitCorners, sizeof lobbyKitCorners/sizeof(Corner)},
    [LobbyBullets] = {399441, 567705, LobbyBulletsHeld, lobbyBulletCorners, sizeof lobbyBulletCorners/sizeof(Corner)},
};
static Model models[sizeof pickups/sizeof pickups[0]];
static uint16_t fontPalette[16];
static enum { Inactive, Stopping, Kneeling, Preparing, Inspecting, Rising, LatchNotice,
              EmptyShelfNotice, EmptyHandNotice, BoilerStopping, BoilerPrompt,
              BoilerStarting, BoilerNoEffect, BoilerRunning, TowerInsideNotice, LobbyFanNotice } phase;
static PickupItem activeItem;
static unsigned ticks, selected, flash, shown;
static unsigned collected;

static Glyph latchGlyphs[24], emptyShelfGlyphs[24], emptyHandGlyphs[48], towerDoorGlyphs[24];
static uint16_t latchCount, emptyShelfCount, emptyHandCount, towerDoorCount;
static Glyph fanGlyphs[2][48];
static uint16_t fanCount[2];
static unsigned fanPage;
static bool unlocked, powered;
static Glyph boilerQuestion[48], boilerNoEffect[24], boilerRunning[24];
static uint16_t boilerCount, boilerLength, boilerNoEffectCount, boilerRunningCount;
// MAP1_S00 callbacks 18 and 19, measured from switch confirmation.
enum { BoilerBlinkAge = 63, BoilerStartAge = 276, BoilerPowerAge = 279, BoilerFinishAge = 372 };

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

static bool loadMessage(FILE *file, Glyph *glyphs, unsigned capacity,
                        uint16_t *glyphCount, uint16_t *messageLength)
{
    uint16_t count, length, palette[16];
    if (fread(&count, 2, 1, file) != 1 || !count || count > capacity ||
        fread(&length, 2, 1, file) != 1 || !length || length > count ||
        fread(palette, sizeof palette, 1, file) != 1 ||
        memcmp(palette, fontPalette, sizeof palette) || !glyphsRead(file, glyphs, count))
        return false;
    *glyphCount = count;
    *messageLength = length;
    return true;
}

static bool loadNotice(FILE *file, Glyph *glyphs, unsigned capacity, uint16_t *glyphCount)
{
    uint16_t count, length;
    if (!loadMessage(file, glyphs, capacity, &count, &length) || count != length) return false;
    *glyphCount = count;
    return true;
}

bool boilerLoad(FILE *file)
{
    return loadMessage(file, boilerQuestion, 48, &boilerCount, &boilerLength) && boilerLength < boilerCount &&
           loadNotice(file, boilerNoEffect, 24, &boilerNoEffectCount) &&
           loadNotice(file, boilerRunning, 24, &boilerRunningCount);
}

void boilerStart(void)
{
    if (!boilerCount || pickupActive()) return;
    ticks = selected = flash = shown = 0;
    phase = BoilerStopping;
}

bool boilerPowered(void) { return powered; }

bool boilerGlow(void)
{
    if (powered) return true;
    return phase == BoilerStarting && (towerFlags() & MoonPlaced) &&
           ticks >= BoilerBlinkAge && (ticks-BoilerBlinkAge)%51 < 27;
}

unsigned boilerVolume(void)
{
    if (roomCurrent() == BasementStair) return powered ? 25 : 0;
    if (roomCurrent() != BoilerRoom || !(towerFlags() & MoonPlaced)) return 0;
    if (phase == BoilerStarting) {
        unsigned volume = 12 + ticks*8/25;
        return volume < 127 ? volume : 127;
    }
    return powered ? 127 : 12;
}

bool towerDoorLoad(FILE *file) { return loadNotice(file, towerDoorGlyphs, 24, &towerDoorCount); }

void towerDoorStart(void)
{
    if (!towerDoorCount || pickupActive()) return;
    shown = 0;
    phase = TowerInsideNotice;
}

bool lobbyFanLoad(FILE *file)
{
    return loadNotice(file, fanGlyphs[0], 48, &fanCount[0]) &&
           loadNotice(file, fanGlyphs[1], 48, &fanCount[1]);
}

void lobbyFanStart(void)
{
    if (!fanCount[0] || pickupActive()) return;
    fanPage = shown = 0;
    phase = LobbyFanNotice;
}

bool latchLoad(FILE *file) { return loadNotice(file, latchGlyphs, 24, &latchCount); }

bool pickupLoadEmptyShelf(FILE *file) { return loadNotice(file, emptyShelfGlyphs, 24, &emptyShelfCount); }

bool pickupLoadEmptyHand(FILE *file) { return loadNotice(file, emptyHandGlyphs, 48, &emptyHandCount); }

bool latchStart(void)
{
    if (unlocked || !latchCount || pickupActive()) return false;
    phase = LatchNotice;
    ticks = shown = 0;
    return true;
}

bool latchUnlocked(void) { return unlocked; }

unsigned pickupFlags(void)
{
    return collected;
}

void pickupRestore(unsigned flags)
{
    collected = flags & PickupFlags;
    unlocked = (flags & LatchOpen) != 0;
    powered = (flags & BoilerPowered) != 0;
}

bool pickupActive(void)
{
    return phase != Inactive;
}

static void preparePickup(PickupItem item)
{
    activeItem = item;
    ticks = selected = flash = shown = 0;
    phase = item == SilverMedallion ? Stopping : Preparing;
}

void pickupStart(void)
{
    if (pickupActive()) return;
    if (roomCurrent() == EquipmentRoom) {
        if (!models[Chemical].loaded || !playerFacingChemical()) return;
        if (collected & ChemicalTaken) {
            phase = EmptyShelfNotice;
            ticks = shown = 0;
        } else {
            preparePickup(Chemical);
        }
        return;
    }
    if (roomCurrent() == ChemistryLab) {
        if (!roomCanDissolveHand() || !statueOpened() || !playerFacingHand()) return;
        if (collected & GoldHeld) {
            phase = EmptyHandNotice;
            ticks = shown = 0;
        } else {
            preparePickup(GoldMedallion);
        }
        return;
    }
    PickupItem first, last;
    if (roomCurrent() == MusicRoom) {
        first = last = SilverMedallion;
    } else if (roomCurrent() == Infirmary) {
        first = HealthDrink;
        last = FirstAidKit;
    } else if (roomCurrent() == OtherworldLobby) {
        first = LobbyKit;
        last = LobbyBullets;
    } else {
        return;
    }
    // Check the props in their original event scan order.
    for (PickupItem item = first; item <= last; item++) {
        if (!models[item].loaded || pickupHidden(item) ||
            !playerFacingPoint(pickups[item].x, pickups[item].z)) continue;
        preparePickup(item);
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
    case LatchNotice:
    case EmptyShelfNotice:
    case EmptyHandNotice:
    case BoilerNoEffect:
    case BoilerRunning:
    case LobbyFanNotice:
        if (shown < fanCount[fanPage])
            shown = (pressed & (KEY_A | KEY_B)) ? fanCount[fanPage] : shown+1;
        else if (pressed & (KEY_A | KEY_B)) {
            shown = 0;
            if (fanPage == 0) fanPage = 1;
            else phase = Inactive;
        }
        break;
    case TowerInsideNotice: {
        // Event_DisplayMapMsgWithSfx delays the first latch glyph by 0.2 s.
        bool latch = phase == LatchNotice;
        unsigned count = latch ? latchCount : phase == EmptyHandNotice ? emptyHandCount :
                         phase == BoilerNoEffect ? boilerNoEffectCount :
                         phase == BoilerRunning ? boilerRunningCount :
                         phase == TowerInsideNotice ? towerDoorCount : emptyShelfCount;
        bool advance = pressed & (KEY_A | KEY_B);
        if (latch && ++ticks < 12) break;
        if (shown < count) {
            shown = advance ? count : shown+1;
        } else if (advance) {
            if (latch) unlocked = true;
            phase = Inactive;
        }
        break;
    }
    case BoilerStopping:
        if (playerStopped()) phase = powered ? BoilerRunning : BoilerPrompt;
        break;
    case BoilerPrompt:
        if (shown < boilerCount) {
            shown = pressed & (KEY_A | KEY_B) ? boilerCount : shown+1;
            if (shown >= boilerLength) shown = boilerCount;
            break;
        }
        flash++;
        if ((pressed & KEY_A) && !(pressed & KEY_B) && !selected) {
            phase = BoilerStarting;
            ticks = shown = 0;
            return PickupBoilerSwitch;
        }
        if (pressed & (KEY_A | KEY_B)) {
            phase = Inactive;
            return PickupCancel;
        }
        if ((pressed & KEY_UP && selected) || (pressed & KEY_DOWN && !selected)) {
            selected = !selected;
            flash = 0;
            return PickupMove;
        }
        break;
    case BoilerStarting:
        ticks++;
        if (!(towerFlags() & MoonPlaced)) {
            if (ticks >= 31) phase = BoilerNoEffect;
        } else {
            if (ticks == BoilerStartAge) return PickupBoilerStart;
            if (ticks == BoilerPowerAge) powered = true;
            if (ticks >= BoilerFinishAge) phase = Inactive;
        }
        break;
    case Stopping:
        if (playerStopped()) phase = Kneeling;
        break;
    case Kneeling:
        if (playerActionDone(PlayerKneel)) {
            ticks = selected = flash = shown = 0;
            phase = Inspecting;
        }
        break;
    case Preparing: {
        // The PS1 loads the item model with player control frozen.
        unsigned delay = activeItem == HealthDrink ? 23 : activeItem == GoldMedallion ? 32 : 27;
        if (++ticks >= delay) {
            ticks = 0;
            phase = Inspecting;
        }
        break;
    }
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
    bool chemical = activeItem == Chemical;
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
    glRotateXi(chemical ? -4096 : 4096);
    int scale = 4096;
    if (age < expanded)
        scale = silver ? (int)(age-2)*4096/15 : (int)age*4096/(chemical ? 21 : 15);
    if (scale > 4096) scale = 4096;
    glScalef32(scale, scale, scale);
    unsigned height = silver || chemical || activeItem == GoldMedallion ? TEXTURE_SIZE_64 :
                      activeItem == LobbyKit || activeItem == LobbyBullets ? TEXTURE_SIZE_128 : TEXTURE_SIZE_256;
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
    unsigned visible = phase >= LatchNotice || phase == Inspecting ? shown : 0;
    const Glyph *text = phase == LatchNotice ? latchGlyphs :
                        phase == EmptyShelfNotice ? emptyShelfGlyphs :
                        phase == EmptyHandNotice ? emptyHandGlyphs : phase == BoilerPrompt ? boilerQuestion :
                        phase == BoilerNoEffect ? boilerNoEffect :
                        phase == BoilerRunning ? boilerRunning :
                        phase == TowerInsideNotice ? towerDoorGlyphs :
                        phase == LobbyFanNotice ? fanGlyphs[fanPage] : models[activeItem].glyphs;
    if (!visible) {
        bgHide(bg);
        lastShown = 0;
        return;
    }
    uint16_t *pixels = bgGetGfxPtr(bg);
    if (!lastShown)
        dmaFillHalfWords(0, pixels, 256*192);
    if (visible > lastShown) glyphsDraw(pixels, text+lastShown, visible-lastShown);
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
