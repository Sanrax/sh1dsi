#ifndef SH1_ROOM_H
#define SH1_ROOM_H
#include <stdbool.h>
#include <stdio.h>
#include "medallion.h"

typedef enum { MusicRoom = 39, EquipmentRoom = 35, ChemistryLab = 34, UpperNorthHall = 33, SchoolCorridor = 32,
               EastCorridor = 31, SouthClassroom = 30, NorthClassroom = 29, OtherworldEastHall = 28, OtherworldSideHall = 27, OtherworldLobby = 26,
               OtherworldNorthHall = 25, OtherworldCourtyard = 24, Courtyard = 23,
               TowerPassage = 22, FarClockTower = 21, ClockTower = 20,
               FirstFloor = 12, OtherworldSouthHall = 11, SouthHall = 13, NorthHall = 16, OtherworldReception = 8, Infirmary = 9,
               BasementStair = 3, BoilerRoom = 2 } SchoolRoom;

typedef struct {
    SchoolRoom room, other;
    int x, z, nx, nz, halfWidth;
} Door;
extern const Door schoolDoors[];
extern const unsigned schoolDoorCount;

SchoolRoom roomCurrent(void);
bool roomHasNormals(void);
void roomTick(void);
bool roomHasHand(void);
bool roomCanDissolveHand(void);
typedef struct { int x, z; } RoomOrigin; // Q12 world coordinates.
RoomOrigin roomOrigin(void);
bool roomLoad(FILE *file, SchoolRoom room);
bool roomLoadShared(FILE *file);
void roomDraw(const Medallion *medal);

#endif
