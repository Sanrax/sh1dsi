#ifndef SH1_PLAYER_H
#define SH1_PLAYER_H
#include <stdbool.h>
#include <stdio.h>

bool playerLoad(FILE *file, bool hasControls);
bool playerReady(void);
bool playerTick(int turn);
void playerWarpForMedal(void);
bool playerFacingPiano(void);
void playerSetCamera(void);
void playerDraw(unsigned firstPalette);

#endif
