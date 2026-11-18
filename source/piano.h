#ifndef SH1_PIANO_H
#define SH1_PIANO_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int8_t thresholds[11];
    uint8_t sequence[5];
} PianoKeyboard;

typedef struct {
    int x, y; /* centered PS1 coordinates, Q12 */
    unsigned matched;
    unsigned waitUs;
    bool checking, solved, waitingForRelease;
} Piano;

// Returns a bank index for the ARM7 sound channel, or -1 for no new sound.
int pianoUpdate(Piano *p, const PianoKeyboard *keyboard, int dx, int dy,
                 bool enter, bool cancel, unsigned stepUs);

#endif
