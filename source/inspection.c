// SPDX-License-Identifier: GPL-3.0-only
// Adapted from the school inspections and MAP1_S02 events 10 and 11.
// Copyright (C) 2026 shdecompilations
#include <nds.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "glyph.h"
#include "inspection.h"
#include "checkpoint.h"
#include "player.h"

typedef struct {
    uint16_t palette[256];
    uint8_t pixels[256*192];
} Background;

typedef struct {
    uint16_t count, length, palette[16];
    Glyph glyphs[48];
} Message;
_Static_assert(offsetof(Message, glyphs) == 36, "inspection message layout");

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

typedef struct {
    int bg;
    uint8_t bank;
    uint16_t control, alpha;
    uint16_t *pixels;
} InspectionOverlay;

static void drawMessageGlyph(uint16_t *screen, const Message *text, unsigned index)
{
    const Glyph *g = &text->glyphs[index];
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

static void blendInspection(unsigned step)
{
    REG_BLDALPHA = step | ((16-step) << 8);
}

static void showMessageAcrossViews(uint16_t *screen, const Message *text, unsigned interval,
                                  const InspectionOverlay *overlay)
{
    unsigned shown = 0, age = 0, elapsed = 0;
    for (;;) {
        unsigned keys = input() & (KEY_A | KEY_B);
        if (overlay && elapsed < 32) blendInspection(++elapsed/2);
        if (shown == text->count && keys) return;
        if (++age < interval && !keys) continue;
        age = 0;
        unsigned goal = keys || shown+2 > text->count ? text->count : shown+2;
        while (shown < goal) {
            drawMessageGlyph(screen, text, shown);
            if (overlay) drawMessageGlyph(overlay->pixels, text, shown);
            shown++;
        }
    }
}

static void showMessage(uint16_t *screen, const Message *text, unsigned interval)
{
    showMessageAcrossViews(screen, text, interval, NULL);
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

static InspectionOverlay openInspectionOverlay(const Background *image)
{
    InspectionOverlay overlay = {.bank = VRAM_D_CR, .control = REG_BLDCNT, .alpha = REG_BLDALPHA};
    vramSetBankD(VRAM_D_MAIN_BG_0x06020000);
    overlay.bg = bgInitHidden(2, BgType_Bmp16, BgSize_B16_256x256, 8, 0);
    bgSetPriority(overlay.bg, 0);
    bgSetPriority(3, 1);
    overlay.pixels = bgGetGfxPtr(overlay.bg);
    drawBackground(overlay.pixels, image);
    return overlay;
}

static void startInspectionBlend(const InspectionOverlay *overlay)
{
    REG_BLDCNT = BLEND_ALPHA | BLEND_SRC_BG2 | BLEND_DST_BG3;
    blendInspection(0);
    bgShow(overlay->bg);
}

static void finishInspectionBlend(const InspectionOverlay *overlay, uint16_t *screen)
{
    input();
    dmaCopy(overlay->pixels, screen, 256*192*2);
    bgHide(overlay->bg);
    REG_BLDCNT = overlay->control;
    REG_BLDALPHA = overlay->alpha;
    VRAM_D_CR = overlay->bank;
    bgInitHidden(2, BgType_Bmp8, BgSize_B8_256x256, 4, 0);
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
    drawBackground(screen, &art->scene);
    InspectionOverlay overlay = openInspectionOverlay(filled);
    free(filled);
    bgShow(3);
    fade(33, false);
    waitFrames(31);

    // Use both bitmap layers so the ARM9 only updates the blend weights.
    playSound(false);
    startInspectionBlend(&overlay);
    for (unsigned step = 1; step <= 16; step++) {
        input();
        blendInspection(step);
    }
    finishInspectionBlend(&overlay, screen);

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

enum { SlotFirst, SlotRepeat, SlotSurprise, SlotClose, SlotMessages };
static bool slotSeen, cardDoorOpen;
static Message insertMessage;
static enum { SlotInactive, SlotInsert, SlotMessage, SlotRecover, SlotBlackout } slotPhase;
static unsigned slotAge, slotShown;

static FILE *openSlot(const char *dir, uint32_t *inspection)
{
    char path[64], magic[4];
    snprintf(path, sizeof path, "%s/cardslot.dat", dir);
    FILE *file = fopen(path, "rb");
    if (!file) return NULL;
    if (fread(magic, sizeof magic, 1, file) == 1 && !memcmp(magic, "SHR1", 4) &&
        fread(inspection, sizeof *inspection, 1, file) == 1 && *inspection >= 8 && *inspection < 16*1024)
        return file;
    fclose(file);
    return NULL;
}

unsigned slotInspect(const char *dir)
{
    typedef struct {
        Background view[2];
        Message text[SlotMessages];
    } SlotArt;
    uint32_t offset;
    SlotArt *art = malloc(sizeof *art);
    FILE *file = art ? openSlot(dir, &offset) : NULL;
    bool ok = file && !fseek(file, offset, SEEK_SET) && fread(art->view, sizeof art->view, 1, file) == 1;
    for (unsigned i = 0; ok && i < SlotMessages; i++) ok = readMessage(file, &art->text[i]);
    if (file) {
        ok = ok && fgetc(file) == EOF && !ferror(file);
        fclose(file);
    }
    if (!ok) {
        free(art);
        return 0;
    }

    fade(40, true);
    bgHide(0);
    uint16_t *screen = openScreen();
    drawBackground(screen, &art->view[0]);
    InspectionOverlay overlay = openInspectionOverlay(&art->view[1]);
    bgShow(3);
    fade(19, false);
    showMessage(screen, &art->text[slotSeen ? SlotRepeat : SlotFirst], 1);
    drawBackground(screen, &art->view[0]);
    startInspectionBlend(&overlay);
    if (!slotSeen) {
        showMessageAcrossViews(screen, &art->text[SlotSurprise], 1, &overlay);
        // Finish the authored transition if the player dismissed the line
        // while the two backgrounds were still blending.
        for (unsigned step = REG_BLDALPHA & 31; step < 16; step++) {
            waitFrames(2);
            blendInspection(step+1);
        }
    } else {
        for (unsigned step = 1; step <= 16; step++) {
            waitFrames(2);
            blendInspection(step);
        }
    }
    finishInspectionBlend(&overlay, screen);
    drawBackground(screen, &art->view[1]);
    showMessage(screen, &art->text[SlotClose], 1);
    fade(23, true);
    closeScreen();
    free(art);
    slotSeen = true;
    return SlotRevealVBlanks;
}

bool slotUnlocked(void) { return cardDoorOpen; }
bool slotActive(void) { return slotPhase != SlotInactive; }

bool slotStart(const char *dir)
{
    uint32_t offset;
    FILE *file = openSlot(dir, &offset);
    if (!file) return false;
    bool ok = playerLoadSlot(file) && readMessage(file, &insertMessage) && ftell(file) == (long)offset;
    fclose(file);
    if (!ok) return false;
    playerWarpForSlot();
    setBrightness(1, 0);
    slotPhase = SlotInsert;
    slotAge = slotShown = 0;
    return true;
}

bool slotTick(unsigned pressed, void (*playSound)(bool unlocked))
{
    slotAge++;
    switch (slotPhase) {
    case SlotInsert:
        if (slotAge < 19) break;
        if (slotAge < 117) {
            if (slotAge & 1) break;
            // The PS1 evaluates the cutscene bones at the 30 Hz draw cadence.
            unsigned age = slotAge-19;
            unsigned blend = age < 8 ? 4096-age*512 : 0;
            unsigned frame = age > 8 ? (age-8)*1024 : 0;
            if (frame > 20*4096) frame = 20*4096;
            playerSlotFrame(frame, blend);
            return true;
        }
        playerSlotFrame(20*4096, 0);
        playSound(false);
        slotPhase = SlotMessage;
        return true;
    case SlotMessage:
        if (pressed & (KEY_A | KEY_B)) {
            if (slotShown == insertMessage.count) {
                slotShown = 0;
                slotPhase = SlotRecover;
                slotAge = 0;
                playSound(true);
            } else slotShown = insertMessage.count;
        } else if (slotShown < insertMessage.count) {
            slotShown++;
        }
        break;
    case SlotRecover:
        if (slotAge >= 46) {
            playerSlotFrame(15*4096, 0);
            slotAge = 0;
            slotPhase = SlotBlackout;
            return true;
        } else if (slotAge >= 11 && (slotAge & 1)) {
            unsigned back = (slotAge-9)*4096*8/60;
            if (back > 5*4096) back = 5*4096;
            playerSlotFrame(20*4096-back, 0);
            return true;
        }
        break;
    case SlotBlackout:
        if (slotAge < 27) {
            setBrightness(1, -(int)(16*slotAge/27));
        } else {
            setBrightness(1, -16);
            playerFinishSlot();
            cardDoorOpen = true;
            slotPhase = SlotInactive;
            return true;
        }
        break;
    case SlotInactive:
        break;
    }
    return false;
}

void slotDrawPrompt(int bg)
{
    static unsigned drawn;
    if (!slotShown) {
        bgHide(bg);
        drawn = 0;
        return;
    }
    uint16_t *screen = bgGetGfxPtr(bg);
    if (!drawn) {
        dmaFillHalfWords(0, screen, 256*192);
        for (unsigned i = 1; i < 16; i++) {
            uint16_t white = insertMessage.palette[i] & 0x7fff;
            unsigned level = white & 31;
            BG_PALETTE[i] = white;
            BG_PALETTE[16+i] = RGB15(level*24/128, level, level*40/128);
        }
    }
    if (slotShown > drawn) glyphsDraw(screen, insertMessage.glyphs+drawn, slotShown-drawn);
    drawn = slotShown;
    bgShow(bg);
}
