#ifndef SH1_PICKUP_H
#define SH1_PICKUP_H
#include "player.h"

typedef enum { SilverMedallion, HealthDrink, FirstAidKit, Chemical, GoldMedallion, LobbyKit, LobbyBullets, PictureCard } PickupItem;
typedef enum { PickupQuiet, PickupMove, PickupCancel, PickupConfirm,
               PickupBoilerSwitch, PickupBoilerStart } PickupSound;

bool latchLoad(FILE *file);
bool westLatchLoad(FILE *file);
bool westLatchStart(bool unlock);
bool westLatchUnlocked(void);
bool towerDoorLoad(FILE *file);
bool lobbyFanLoad(FILE *file);
void lobbyFanStart(void);
void towerDoorStart(void);
bool boilerLoad(FILE *file);
void boilerStart(void);
bool boilerPowered(void);
bool boilerGlow(void);
unsigned boilerVolume(void);
bool pickupLoadEmptyShelf(FILE *file);
bool pickupLoadEmptyHand(FILE *file);
bool pickupLoadScatteredCards(FILE *file);
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
