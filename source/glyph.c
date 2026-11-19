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
