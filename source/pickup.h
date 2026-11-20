#ifndef SH1_PICKUP_H
#define SH1_PICKUP_H
#include "player.h"

typedef enum { SilverMedallion, HealthDrink, FirstAidKit, Chemical } PickupItem;
typedef enum { PickupQuiet, PickupMove, PickupCancel, PickupConfirm } PickupSound;

bool latchLoad(FILE *file);
bool pickupLoadEmptyShelf(FILE *file);
bool latchStart(void);
unsigned pickupFlags(void);
void pickupRestore(unsigned flags);
bool latchUnlocked(void);
bool pickupLoad(FILE *file, PickupItem item, unsigned textureOffset, unsigned palette);
void pickupStart(void);
bool pickupActive(void);
PlayerAction pickupAction(void);
PickupSound pickupTick(unsigned pressed);
bool pickupHidden(PickupItem item);
void pickupDrawItem(void);
void pickupDrawPrompt(int bg);
#endif
