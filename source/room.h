#ifndef SH1_ROOM_H
#define SH1_ROOM_H
#include <stdbool.h>
#include <stdio.h>
#include "medallion.h"

typedef enum { MusicRoom = 39, EquipmentRoom = 35, ChemistryLab = 34, UpperNorthHall = 33, SchoolCorridor = 32,
               EastCorridor = 31, Courtyard = 23,
               FirstFloor = 12, SouthHall = 13, NorthHall = 16, Infirmary = 9 } SchoolRoom;

typedef struct {
    SchoolRoom room, other;
    int x, z, nx, nz, halfWidth;
} Door;
extern const Door schoolDoors[];
extern const unsigned schoolDoorCount;

SchoolRoom roomCurrent(void);
bool roomHasHand(void);
bool roomCanDissolveHand(void);
typedef struct { int x, z; } RoomOrigin; // Q12 world coordinates.
RoomOrigin roomOrigin(void);
bool roomLoad(FILE *file, SchoolRoom room);
bool roomLoadShared(FILE *file);
void roomDraw(const Medallion *medal);

#endif
