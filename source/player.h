#ifndef SH1_PLAYER_H
#define SH1_PLAYER_H
#include <stdbool.h>
#include <stdio.h>

bool playerLoad(FILE *file);
bool playerReady(void);
bool playerTick(void);
void playerDraw(unsigned firstPalette, bool afterPuzzle);

#endif
