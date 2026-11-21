#ifndef SH1_PLAYER_H
#define SH1_PLAYER_H
#include "room.h"
#include "checkpoint.h"
#include "inspection.h"
#include <stdio.h>

bool playerLoad(FILE *file, unsigned version);
bool playerReady(void);
typedef enum { PlayerMove, PlayerKneel, PlayerRise, PlayerPour, PlayerLookAround, PlayerScriptedWalk } PlayerAction;
typedef struct {
    int x, y, z, watchX, watchY, watchZ;
} CameraPose;
bool playerLoadCourtyardLook(FILE *file);
void playerScriptedPose(int worldX, int worldZ, int heading);
void playerFinishOtherworld(bool skipped);
bool playerTick(int turn, int walk, PlayerAction action);
bool playerStopped(void);
bool playerActionDone(PlayerAction action);
bool playerFacingPoint(int worldX, int worldZ);
void playerWarpForMedal(void);
void playerWarpForPour(void);
bool playerLoadPour(FILE *file);
bool playerLoadLadder(FILE *file, bool ascent);
bool playerStartAscent(void);
bool playerLadderActive(void);
bool playerLadderTick(bool skip, SchoolRoom *arrival);
enum { LadderRevealVBlanks = 46 };
bool playerFacingPiano(void);
bool playerFacingNotebook(void);
bool playerFacingBoiler(void);
bool playerFacingTowerExit(void);
bool playerFacingChemical(void);
bool playerFacingHand(void);
TowerPoint playerFacingTower(void);
void playerSavePose(Checkpoint *point);
void playerRestorePose(const Checkpoint *point);
typedef struct StairExit StairExit;
typedef struct {
    SchoolRoom room;
    const StairExit *stairs;
} RoomExit;
RoomExit playerDestination(bool action);
void playerEnterRoom(SchoolRoom from, const StairExit *stairs);
int playerSetCamera(void);
void playerDraw(unsigned firstPalette);

#endif
