#ifndef SH1_PLAYER_H
#define SH1_PLAYER_H
#include "room.h"
#include "checkpoint.h"
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
bool playerFacingNotebook(void);
void playerSavePose(Checkpoint *point);
void playerRestorePose(const Checkpoint *point);
SchoolRoom playerDestination(bool action);
void playerEnterRoom(SchoolRoom from);
int playerSetCamera(void);
void playerDraw(unsigned firstPalette);

#endif
