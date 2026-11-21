#include "glyph.h"

bool glyphsRead(FILE *file, Glyph *data, unsigned count)
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

void glyphsDraw(uint16_t *pixels, const Glyph *text, unsigned count)
{
    for (unsigned i = 0; i < count; i++) {
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
}
