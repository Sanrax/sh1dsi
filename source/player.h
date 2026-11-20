#ifndef SH1_PLAYER_H
#define SH1_PLAYER_H
#include "room.h"
#include "checkpoint.h"
#include "inspection.h"
#include <stdio.h>

bool playerLoad(FILE *file, unsigned version);
bool playerReady(void);
typedef enum { PlayerMove, PlayerKneel, PlayerRise, PlayerPour } PlayerAction;
bool playerTick(int turn, int walk, PlayerAction action);
bool playerStopped(void);
bool playerActionDone(PlayerAction action);
bool playerFacingPoint(int worldX, int worldZ);
void playerWarpForMedal(void);
void playerWarpForPour(void);
bool playerLoadPour(FILE *file);
bool playerFacingPiano(void);
bool playerFacingNotebook(void);
bool playerFacingChemical(void);
bool playerFacingHand(void);
TowerPoint playerFacingTower(void);
void playerSavePose(Checkpoint *point);
void playerRestorePose(const Checkpoint *point);
SchoolRoom playerDestination(bool action);
void playerEnterRoom(SchoolRoom from);
int playerSetCamera(void);
void playerDraw(unsigned firstPalette);

#endif
