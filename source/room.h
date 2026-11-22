#ifndef SH1_ROOM_H
#define SH1_ROOM_H
#include <stdbool.h>
#include <stdio.h>
#include "medallion.h"

typedef enum { MusicRoom = 39, EquipmentRoom = 35, ChemistryLab = 34, UpperNorthHall = 33, SchoolCorridor = 32,
               EastCorridor = 31, SouthClassroom = 30, NorthClassroom = 29, OtherworldEastHall = 28, OtherworldSideHall = 27, OtherworldLobby = 26,
               OtherworldNorthHall = 25, OtherworldCourtyard = 24, Courtyard = 23,
               TowerPassage = 22, FarClockTower = 21, ClockTower = 20,
               FirstFloor = 12, OtherworldSouthHall = 11, SouthHall = 13, NorthHall = 16, OtherworldWestHall = 10, OtherworldReception = 8, Infirmary = 9,
               OtherworldRestroom = 14, OtherworldUpperRestroom = 15, OtherworldUpperWestHall = 17, OtherworldStorage = 19,
               TeachersSouth = 6, TeachersNorth = 7, BasementStair = 3, BoilerRoom = 2 } SchoolRoom;

static inline bool roomIsTeachersRoom(SchoolRoom room)
{
    return room == TeachersSouth || room == TeachersNorth;
}

static inline bool roomIsRestroom(SchoolRoom room)
{
    return room == OtherworldRestroom || room == OtherworldUpperRestroom;
}

static inline bool roomUsesCardDoor(SchoolRoom from, SchoolRoom to)
{
    return (from == OtherworldReception && to == OtherworldWestHall) ||
           (from == OtherworldWestHall && to == OtherworldReception);
}

static inline bool roomUsesWestLatch(SchoolRoom from, SchoolRoom to)
{
    return (from == OtherworldWestHall && to == OtherworldNorthHall) ||
           (from == OtherworldNorthHall && to == OtherworldWestHall);
}

typedef struct {
    SchoolRoom room, other;
    int x, z, nx, nz, halfWidth;
} Door;
extern const unsigned roomDoorCount;
const Door *roomDoor(unsigned index);
const Door *roomDoorArrival(unsigned index);

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
