#ifndef SH1_GLYPH_H
#define SH1_GLYPH_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint8_t x, y, ink, reserved, mask[14][11];
} Glyph;

bool glyphsRead(FILE *file, Glyph *data, unsigned count);
void glyphsDraw(uint16_t *pixels, const Glyph *text, unsigned count);

#endif
