#ifndef SH1_PICKUP_H
#define SH1_PICKUP_H
#include "player.h"

bool pickupLoad(FILE *file, unsigned textureOffset, unsigned palette);
void pickupStart(void);
bool pickupActive(void);
PlayerAction pickupAction(void);
void pickupTick(unsigned pressed);
bool pickupFloorHidden(void);
void pickupDrawItem(void);
void pickupDrawPrompt(int bg);
#endif
