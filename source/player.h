#ifndef SH1_PLAYER_H
#define SH1_PLAYER_H
#include <stdbool.h>
#include <stdio.h>

bool playerLoad(FILE *file, unsigned version);
bool playerReady(void);
typedef enum { PlayerMove, PlayerKneel, PlayerRise } PlayerAction;
bool playerTick(int turn, int walk, PlayerAction action);
bool playerStopped(void);
bool playerActionDone(PlayerAction action);
bool playerFacingMedallion(void);
void playerWarpForMedal(void);
bool playerFacingPiano(void);
bool playerFacingDoor(void);
void playerEnterRoom(void);
int playerSetCamera(void);
void playerDraw(unsigned firstPalette);

#endif
