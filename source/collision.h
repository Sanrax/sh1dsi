#ifndef SH1_COLLISION_H
#define SH1_COLLISION_H
#include "room.h"
#include <stdio.h>

typedef enum { NoFloorData, LegacyWalkways, LegacyCourtyardFloors, CountedFloorPlanes } FloorLayout;
typedef enum { LegacyEdges, ExpandedEdges, CountedEdges } EdgeLayout;
bool collisionLoad(FILE *file, SchoolRoom room, EdgeLayout edgeLayout, FloorLayout floorLayout);
typedef struct { int height, slopeX, slopeZ; } Floor;
Floor collisionFloor(int x, int z);
void collisionMove(int *worldX, int *worldZ, int dx, int dz);

#endif
