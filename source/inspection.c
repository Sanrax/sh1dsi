// SPDX-License-Identifier: GPL-3.0-only
// Adapted from the school inspections and Event_DisplayMapMsgWithBg.
// Copyright (C) 2026 shdecompilations
#include <nds.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "glyph.h"
#include "inspection.h"
#include "checkpoint.h"

typedef struct {
    uint16_t palette[256];
    uint8_t pixels[256*192];
} Background;

typedef struct {
    uint16_t count, length, palette[16];
    Glyph glyphs[40];
} Message;
_Static_assert(offsetof(Message, glyphs) == 36, "tower message layout");

typedef struct {
    Background scene;
    Message lock, inscription;
} Art;
enum { ClockTen, EmptySun, EmptyMoon, FilledSun, FilledMoon, ClockTwelve, ClockFive };
static unsigned placed;

unsigned towerFlags(void) { return placed; }
void towerRestore(unsigned flags) { placed = flags & TowerFlags; }

bool towerCanPlace(TowerPoint point, unsigned items)
{
    if (point == GoldenSun) return (items & GoldHeld) && !(placed & SunPlaced);
    if (point == SilverMoon) return (items & SilverHeld) && !(placed & MoonPlaced);
    return false;
}

static bool readMessage(FILE *file, Message *text)
{
    if (fread(text, offsetof(Message, glyphs), 1, file) != 1 ||
        !text->count || text->count > sizeof text->glyphs/sizeof text->glyphs[0] || text->count != text->length)
        return false;
    return glyphsRead(file, text->glyphs, text->count);
}

static bool readTowerMessage(FILE *file, uint32_t offset, unsigned first, Message *text)
{
    return offset >= first && offset < first+16*1024 &&
           !fseek(file, offset, SEEK_SET) && readMessage(file, text);
}

static bool readTowerImage(FILE *file, unsigned header, unsigned image, Background *out)
{
    return !fseek(file, header+image*sizeof *out, SEEK_SET) && fread(out, sizeof *out, 1, file) == 1;
}

static bool readTower(const char *dir, TowerPoint point, Art *art, Background *filled)
{
    char path[64], magic[4];
    snprintf(path, sizeof path, "%s/tower.dat", dir);
    FILE *file = fopen(path, "rb");
    if (!file) return false;
    bool ok = fread(magic, sizeof magic, 1, file) == 1 && !memcmp(magic, "SHT", 3) &&
              (magic[3] == '1' || magic[3] == '2');
    bool updated = ok && magic[3] == '2';
    unsigned count = updated ? 6 : 4;
    uint32_t messages[6];
    unsigned header = sizeof magic + count*sizeof messages[0];
    unsigned firstMessage = header + (updated ? 7 : 3)*sizeof(Background);
    unsigned image = point-TowerDoor, message = point;
    if (updated && !filled) {
        if (point == TowerDoor) {
            if (placed & MoonPlaced) { image = ClockFive; message = 5; }
            else if (placed & SunPlaced) { image = ClockTwelve; message = 4; }
        } else if (point == GoldenSun && (placed & SunPlaced)) image = FilledSun;
        else if (point == SilverMoon && (placed & MoonPlaced)) image = FilledMoon;
    }
    ok = ok && (!filled || updated) && fread(messages, sizeof messages[0], count, file) == count &&
         readTowerImage(file, header, image, &art->scene) &&
         readTowerMessage(file, messages[message], firstMessage, &art->inscription);
    if (ok && point == TowerDoor) ok = readTowerMessage(file, messages[0], firstMessage, &art->lock);
    if (ok && filled) ok = readTowerImage(file, header, point == GoldenSun ? FilledSun : FilledMoon, filled);
    fclose(file);
    return ok;
}

static unsigned input(void)
{
    swiWaitForVBlank();
    scanKeys();
    return keysDown();
}

static void fade(unsigned frames, bool darken)
{
    for (unsigned i = 1; i <= frames; i++) {
        input();
        int shade = 16*i/frames;
        setBrightness(1, darken ? -shade : shade-16);
    }
}

static void waitFrames(unsigned frames)
{
    for (unsigned i = 0; i < frames; i++) input();
}

static void showMessage(uint16_t *screen, const Message *text, unsigned interval)
{
    unsigned shown = 0, age = 0;
    for (;;) {
        unsigned keys = input() & (KEY_A | KEY_B);
        if (shown == text->count && keys) return;
        if (++age < interval && !keys) continue;
        age = 0;
        unsigned goal = keys || shown+2 > text->count ? text->count : shown+2;
        while (shown < goal) {
            const Glyph *g = &text->glyphs[shown++];
            for (unsigned y = 0; y < 14; y++) {
                uint16_t *row = screen+(g->y+y)*256+g->x;
                for (unsigned x = 0; x < 11; x++) {
                    unsigned ink = g->mask[y][x];
                    if (!ink) continue;
                    uint16_t color = text->palette[ink];
                    if (g->ink == 2) {
                        // MAP_MSG_CODE_COLOR 3 uses RGB modulation 8,184,96.
                        unsigned level = color & 31, green = level*184/128;
                        color = RGB15(level*8/128, green > 31 ? 31 : green, level*96/128);
                    }
                    row[x] = color | BIT(15);
                }
            }
        }
    }
}

static void drawBackground(uint16_t *screen, const Background *image)
{
    uint16_t dithered[256];
    for (unsigned i = 0; i < 256; i++) {
        unsigned c = image->palette[i];
        unsigned r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
        dithered[i] = RGB15(r-(r != 0), g-(g != 0), b-(b != 0));
    }
    // At neutral modulation only the negative PS1 dither entries change
    // a 5-bit channel. Use the phase of the sampled PS1 screen pixel.
    for (unsigned y = 0; y < 192; y++) {
        unsigned v = (2*y+1)*5/8;
        for (unsigned x = 0; x < 256; x++) {
            unsigned u = (2*x+1)*5/8;
            unsigned index = image->pixels[y*256+x];
            screen[y*256+x] = BIT(15) | (((u^v)&1) ? image->palette[index] : dithered[index]);
        }
    }
}

static bool readHand(const char *dir, Art *art)
{
    char path[64], magic[4];
    snprintf(path, sizeof path, "%s/hand.dat", dir);
    FILE *file = fopen(path, "rb");
    if (!file) return false;
    bool ok = fread(magic, sizeof magic, 1, file) == 1 && !memcmp(magic, "SHV1", 4) &&
              fread(&art->scene, sizeof art->scene, 1, file) == 1 &&
              readMessage(file, &art->inscription) && fgetc(file) == EOF && !ferror(file);
    fclose(file);
    return ok;
}

static uint16_t *openScreen(void)
{
    int bg = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    bgSetPriority(bg, 0);
    return bgGetGfxPtr(bg);
}

static void closeScreen(void)
{
    bgHide(3);
    int bg = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
    bgSetPriority(bg, 0);
    bgHide(bg);
    bgShow(0);
}

static void inspect(const Art *art, bool locked, unsigned roomFade, unsigned imageFade)
{
    uint16_t *screen = openScreen();
    if (locked) {
        // MAP1_S00 event 10 waits 0.3 s before the room message.
        waitFrames(21);
        dmaFillHalfWords(0, screen, 256*192*2);
        bgShow(3);
        showMessage(screen, &art->lock, 3);
        bgHide(3);
    }

    fade(roomFade, true);
    bgHide(0);
    drawBackground(screen, &art->scene);
    bgShow(3);
    fade(imageFade, false);
    showMessage(screen, &art->inscription, 1);
    fade(imageFade, true);

    closeScreen();
}

unsigned towerInspect(const char *dir, TowerPoint point)
{
    Art *art = malloc(sizeof *art);
    bool ok = art && readTower(dir, point, art, NULL);
    // USA trace from the prompt to black, then the image to full light.
    unsigned roomFade = point == TowerDoor ? 60 : 57;
    if (ok) inspect(art, point == TowerDoor, roomFade, 33);
    free(art);
    return ok ? roomFade : 0;
}

unsigned towerPlace(const char *dir, TowerPoint point, void (*playSound)(bool chime))
{
    Art *art = malloc(sizeof *art);
    Background *filled = malloc(sizeof *filled);
    if (!art || !filled || !readTower(dir, point, art, filled)) {
        free(filled);
        free(art);
        return 0;
    }
    fade(61, true);
    bgHide(0);
    uint16_t *screen = openScreen();
    uint8_t bank = VRAM_D_CR;
    vramSetBankD(VRAM_D_MAIN_BG_0x06020000);
    int overlay = bgInit(2, BgType_Bmp16, BgSize_B16_256x256, 8, 0);
    bgHide(overlay);
    bgSetPriority(overlay, 0);
    bgSetPriority(3, 1);
    drawBackground(screen, &art->scene);
    drawBackground(bgGetGfxPtr(overlay), filled);
    free(filled);
    bgShow(3);
    fade(33, false);
    waitFrames(31);

    // The PS1 subtracts a weighted empty view and adds the filled view.
    // Use both bitmap layers so the ARM9 only updates the blend weights.
    playSound(false);
    uint16_t blend = REG_BLDCNT, alpha = REG_BLDALPHA;
    REG_BLDCNT = BLEND_ALPHA | BLEND_SRC_BG2 | BLEND_DST_BG3;
    REG_BLDALPHA = 16 << 8;
    bgShow(overlay);
    for (unsigned step = 1; step <= 16; step++) {
        input();
        REG_BLDALPHA = step | ((16-step) << 8);
    }
    input();
    dmaCopy(bgGetGfxPtr(overlay), screen, 256*192*2);
    bgHide(overlay);
    REG_BLDCNT = blend;
    REG_BLDALPHA = alpha;
    VRAM_D_CR = bank;

    playSound(true);
    waitFrames(60);
    showMessage(screen, &art->inscription, 1);
    fade(33, true);
    closeScreen();
    placed |= point == GoldenSun ? SunPlaced : MoonPlaced;
    free(art);
    return 57;
}

unsigned handInspect(const char *dir)
{
    Art *art = malloc(sizeof *art);
    bool ok = art && readHand(dir, art);
    if (ok) inspect(art, false, 57, 43);
    free(art);
    return ok ? 33 : 0;
}
