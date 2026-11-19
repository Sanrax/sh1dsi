#ifndef SH1_ROOM_H
#define SH1_ROOM_H
#include <stdbool.h>
#include <stdio.h>
#include "medallion.h"

typedef enum { MusicRoom = 39, SchoolCorridor = 32, EastCorridor = 31, FirstFloor = 12, SouthHall = 13, Infirmary = 9 } SchoolRoom;
SchoolRoom roomCurrent(void);
int roomOriginX(void);
int roomOriginZ(void);
bool roomLoad(FILE *file, SchoolRoom room);
void roomDraw(const Medallion *medal);

#endif
