#ifndef SH1_PICKUP_H
#define SH1_PICKUP_H
#include "player.h"

typedef enum { SilverMedallion, HealthDrink, FirstAidKit } PickupItem;

bool latchLoad(FILE *file);
bool latchStart(void);
unsigned pickupFlags(void);
void pickupRestore(unsigned flags);
bool latchActive(void);
bool latchUnlocked(void);
void latchTick(unsigned pressed);
bool pickupLoad(FILE *file, PickupItem item, unsigned textureOffset, unsigned palette);
void pickupStart(void);
bool pickupActive(void);
PlayerAction pickupAction(void);
void pickupTick(unsigned pressed);
bool pickupHidden(PickupItem item);
void pickupDrawItem(void);
void pickupDrawPrompt(int bg);
#endif
