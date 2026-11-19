#ifndef SH1_ROOM_H
#define SH1_ROOM_H
#include <stdbool.h>
#include <stdio.h>
#include "medallion.h"

typedef enum { MusicRoom = 39, SchoolCorridor = 32, EastCorridor = 31, Courtyard = 23,
               FirstFloor = 12, SouthHall = 13, NorthHall = 16, Infirmary = 9 } SchoolRoom;

typedef struct {
    SchoolRoom room, other;
    int x, z, nx, nz, halfWidth;
} Door;
extern const Door schoolDoors[];
extern const unsigned schoolDoorCount;

SchoolRoom roomCurrent(void);
int roomOriginX(SchoolRoom room);
int roomOriginZ(SchoolRoom room);
bool roomLoad(FILE *file, SchoolRoom room);
void roomDraw(const Medallion *medal);

#endif
