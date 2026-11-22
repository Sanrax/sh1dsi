// The first visit follows SysState_SaveMenu_Update. The save screen uses
// the USA background and 12x16 font at the native 4:3 pixel scale.
#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "notebook.h"

typedef struct {
    char magic[4], intro[128];
    uint8_t widths[84], font[84][16][6];
    uint16_t ink[16], colors[256];
    uint8_t image[256*192];
} Art;
_Static_assert(sizeof(Art) == 57976, "notebook file layout");

static Art *readArt(const char *dir)
{
    char path[64];
    snprintf(path, sizeof path, "%s/notebook.dat", dir);
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    Art *a = malloc(sizeof *a);
    bool ok = a && fread(a, sizeof *a, 1, f) == 1 && !memcmp(a->magic, "SHN1", 4) &&
              memchr(a->intro, 0, sizeof a->intro) && fgetc(f) == EOF && !ferror(f);
    fclose(f);
    if (ok) return a;
    free(a);
    return NULL;
}

static unsigned advance(const Art *a, unsigned c)
{
    return (c >= 39 && c <= 122) ? a->widths[c-39] : 6;
}

// Coordinates are in the original 320x240 image. Sample each LCD pixel's
// center so moving a glyph by one PS1 pixel preserves the correct bearing.
static void glyph(const Art *a, uint16_t *screen, unsigned c, int x, int y, bool selected)
{
    if (c < 39 || c > 122 || x < 0 || y < 0 || x+12 > 320 || y+16 > 240) return;
    for (int v = 4*y/5; v < (4*(y+16)+4)/5; v++) {
        int sy = (2*v+1)*5/8-y;
        if ((unsigned)sy >= 16) continue;
        for (int u = 4*x/5; u < (4*(x+12)+4)/5; u++) {
            int sx = (2*u+1)*5/8-x;
            if ((unsigned)sx >= 12) continue;
            unsigned ink = (a->font[c-39][sy][sx/2] >> (4*(sx&1))) & 15;
            if (ink) screen[v*256+u] = BIT(15) | (selected ? a->ink[ink] & 31 : a->ink[ink]);
        }
    }
}

static void text(const Art *a, uint16_t *screen, const char *s, int x, int y, bool selected)
{
    for (; *s; s++) {
        glyph(a, screen, (unsigned char)*s, x, y, selected);
        x += advance(a, (unsigned char)*s);
    }
}

static unsigned input(void)
{
    swiWaitForVBlank();
    scanKeys();
    return keysDown();
}

static void showIntro(const Art *a, uint16_t *screen)
{
    int width = 0, longest = 0, y = 196;
    for (const char *s = a->intro; *s; s++) {
        if (*s == '\n') {
            width = 0;
            y -= 16;
        } else {
            width += advance(a, (unsigned char)*s);
            if (width > longest) longest = width;
        }
    }
    int left = 160-longest/2, x = left;
    const char *s = a->intro;
    bool reveal = false;
    for (;;) {
        unsigned keys = input();
        if (!*s && (keys & (KEY_A|KEY_B))) break;
        reveal |= (keys & (KEY_A|KEY_B)) != 0;
        while (*s) {
            unsigned c = (unsigned char)*s++;
            if (c == '\n') { y += 16; x = left; continue; }
            glyph(a, screen, c, x, y, false);
            x += advance(a, c);
            if (!reveal && c != ' ') break;
        }
    }
}

static ARM_CODE void page(const Art *a, uint16_t *screen)
{
    // The bitmap base is word-aligned. Set the opaque bit on both pixels.
    volatile uint32_t *out = (volatile uint32_t *)screen;
    for (unsigned i = 0; i < sizeof a->image; i += 2) {
        uint32_t left = a->colors[a->image[i]];
        uint32_t right = a->colors[a->image[i+1]];
        *out++ = left | (right << 16) | 0x80008000u;
    }
}

typedef enum { Browse, Confirm, WriteError, Saved } MenuState;

NotebookResult notebookMenu(const char *dir, bool loading, Checkpoint *point)
{
    Checkpoint slots[SaveSlots];
    SlotState states[SaveSlots];
    unsigned entries[SaveSlots], n = 0, empty = SaveSlots, cursor = 0;
    for (unsigned i = 0; i < SaveSlots; i++) {
        states[i] = checkpointRead(dir, i, &slots[i]);
        if (states[i] == SlotEmpty) {
            if (empty == SaveSlots) empty = i;
        } else if (states[i] != SlotUnavailable || !loading) {
            entries[n++] = i;
        }
    }
    if (!loading && empty != SaveSlots) entries[n++] = empty;
    if (loading) {
        // Highlight the most recently saved point in a progressing playthrough.
        for (unsigned i = 0; i < n; i++)
            if (states[entries[i]] == SlotReady && (states[entries[cursor]] != SlotReady ||
                slots[entries[i]].saves > slots[entries[cursor]].saves)) cursor = i;
    }
    if (!n) return NotebookClosed;
    Art *a = readArt(dir);
    if (!a) return NotebookMissingArt;
    int bg = bgInitHidden(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    bgSetPriority(bg, 0);
    uint16_t *screen = bgGetGfxPtr(bg);
    if (!loading && !(point->flags & NotebookRead)) {
        dmaFillHalfWords(0, screen, 256*192*2);
        bgShow(bg);
        showIntro(a, screen);
        point->flags |= NotebookRead;
    }
    bgHide(0);
    NotebookResult result = NotebookClosed;
    MenuState state = Browse;
    bool draw = true, yes = false;
    for (;;) {
        unsigned slot = entries[cursor];
        if (draw) {
            page(a, screen);
            text(a, screen, loading ? "Load" : "Save", 24, 16, false);
            unsigned top = cursor > 4 ? cursor-4 : 0;
            for (unsigned i = top; i < n && i < top+5; i++) {
                unsigned id = entries[i];
                char line[32];
                if (states[id] == SlotReady) snprintf(line, sizeof line, "%u  Infirmary", id+1);
                else snprintf(line, sizeof line, "%s", states[id] == SlotEmpty ? "New save" :
                              states[id] == SlotDamaged ? "Damaged data" : "Card unavailable");
                text(a, screen, line, 26, 53+20*(i-top), i == cursor);
            }
            if (states[slot] == SlotReady) {
                char info[40];
                snprintf(info, sizeof info, "Save %lu", (unsigned long)slots[slot].saves);
                text(a, screen, info, 170, 54, false);
                unsigned seconds = (uint64_t)slots[slot].ticks*16683/1000000;
                snprintf(info, sizeof info, "%u:%02u:%02u", seconds/3600, seconds/60%60, seconds%60);
                text(a, screen, info, 170, 76, false);
            }
            if (state == Confirm) {
                text(a, screen, states[slot] == SlotEmpty ? "Create new save?" : "Overwrite this save?",
                     20, 174, false);
                text(a, screen, "Yes", 72, 197, yes);
                text(a, screen, "No", 166, 197, !yes);
            } else {
                const char *hint = state == WriteError ? "Unable to save" : state == Saved ? "Saved" :
                                   loading ? "A Load   B New game" : "A Save   B Return";
                text(a, screen, hint, 20, 195, false);
            }
            bgShow(bg);
            draw = false;
        }
        unsigned keys = input();
        if (!keys) continue;
        draw = true;
        if (state == WriteError || state == Saved) {
            if (keys & KEY_B) break;
            if (keys & KEY_A) state = Browse;
            continue;
        }
        if (state == Confirm) {
            if (keys & (KEY_LEFT|KEY_RIGHT)) yes = !yes;
            if (keys & KEY_B || ((keys & KEY_A) && !yes)) { state = Browse; continue; }
            if (!(keys & KEY_A)) continue;
        } else {
            if (keys & KEY_B) break;
            if ((keys & KEY_UP) && cursor) cursor--;
            if ((keys & KEY_DOWN) && cursor+1 < n) cursor++;
            if (slot != entries[cursor] || !(keys & KEY_A)) continue;
            if (states[slot] == SlotUnavailable || (loading && states[slot] == SlotDamaged)) continue;
            if (loading) {
                *point = slots[slot];
                result = NotebookLoaded;
                break;
            }
            state = Confirm;
            yes = states[slot] == SlotEmpty;
            continue;
        }
        Checkpoint next = *point;
        if (next.saves != UINT32_MAX) next.saves++;
        if (!checkpointWrite(dir, slot, &next)) { state = WriteError; continue; }
        *point = slots[slot] = next;
        states[slot] = SlotReady;
        if (slot == empty) {
            for (empty = 0; empty < SaveSlots && states[empty] != SlotEmpty; empty++) {}
            if (empty < SaveSlots) entries[n++] = empty;
        }
        state = Saved;
    }
    bgInitHidden(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
    bgSetPriority(bg, 0);
    free(a);
    return result;
}
