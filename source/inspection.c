// SPDX-License-Identifier: GPL-3.0-only
// Adapted from the school inspections and Event_DisplayMapMsgWithBg.
// Copyright (C) 2026 shdecompilations
#include <nds.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "glyph.h"
#include "inspection.h"

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

static bool readMessage(FILE *file, Message *text)
{
    if (fread(text, offsetof(Message, glyphs), 1, file) != 1 ||
        !text->count || text->count > sizeof text->glyphs/sizeof text->glyphs[0] || text->count != text->length)
        return false;
    return glyphsRead(file, text->glyphs, text->count);
}

static bool readTowerMessage(FILE *file, uint32_t offset, Message *text)
{
    return offset >= 20+3*sizeof(Background) && offset < 200*1024 &&
           !fseek(file, offset, SEEK_SET) && readMessage(file, text);
}

static bool readTower(const char *dir, TowerPoint point, Art *art)
{
    char path[64];
    snprintf(path, sizeof path, "%s/tower.dat", dir);
    FILE *file = fopen(path, "rb");
    if (!file) return false;
    struct { char magic[4]; uint32_t text[4]; } header;
    _Static_assert(sizeof header == 20, "tower file layout");
    bool ok = fread(&header, sizeof header, 1, file) == 1 && !memcmp(header.magic, "SHT1", 4) &&
              !fseek(file, sizeof header + (point-TowerDoor)*sizeof(Background), SEEK_SET) &&
              fread(&art->scene, sizeof art->scene, 1, file) == 1 &&
              readTowerMessage(file, header.text[point], &art->inscription);
    if (ok && point == TowerDoor) ok = readTowerMessage(file, header.text[0], &art->lock);
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

static void inspect(const Art *art, bool locked, unsigned roomFade, unsigned imageFade)
{
    int bg = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    bgSetPriority(bg, 0);
    uint16_t *screen = bgGetGfxPtr(bg);
    if (locked) {
        // MAP1_S00 event 10 waits 0.3 s before the room message.
        for (unsigned i = 0; i < 21; i++) input();
        dmaFillHalfWords(0, screen, 256*192*2);
        bgShow(bg);
        showMessage(screen, &art->lock, 3);
        bgHide(bg);
    }

    fade(roomFade, true);
    bgHide(0);
    drawBackground(screen, &art->scene);
    bgShow(bg);
    fade(imageFade, false);
    showMessage(screen, &art->inscription, 1);
    fade(imageFade, true);

    bgHide(bg);
    bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
    bgSetPriority(bg, 0);
    bgHide(bg);
    bgShow(0);
}

unsigned towerInspect(const char *dir, TowerPoint point)
{
    Art *art = malloc(sizeof *art);
    bool ok = art && readTower(dir, point, art);
    // USA trace from the prompt to black, then the image to full light.
    unsigned roomFade = point == TowerDoor ? 60 : 57;
    if (ok) inspect(art, point == TowerDoor, roomFade, 33);
    free(art);
    return ok ? roomFade : 0;
}

unsigned handInspect(const char *dir)
{
    Art *art = malloc(sizeof *art);
    bool ok = art && readHand(dir, art);
    if (ok) inspect(art, false, 33, 42);
    free(art);
    return ok ? 33 : 0;
}
