// Local XZ collision on the school floors. Edges and tangent vectors
// come from the IPD splits used by the original character query.
#include <nds.h>
#include "collision.h"

typedef struct {
    int ax, az, bx, bz;
    int16_t tx, tz;
} Edge;
static Edge edges[51];
static unsigned edgeCount;
enum { Radius = 1228 }; // Harry's 0.3-metre outer cylinder, Q12.

bool collisionLoad(FILE *file, SchoolRoom room)
{
    switch (room) {
    case FirstFloor: edgeCount = 39; break;
    case SouthHall:
    case EastCorridor: edgeCount = 43; break;
    case SchoolCorridor: edgeCount = 20; break;
    case MusicRoom: edgeCount = 51; break;
    }
    int scale = room == MusicRoom ? 1 : 16;
    // The first-floor south wall extends 0.21 m into the adjoining chunk.
    int maxZ = room == FirstFloor ? 21*4096 : 20*4096;
    for (unsigned i = 0; i < edgeCount; i++) {
        int16_t in[6];
        if (fread(in, sizeof in, 1, file) != 1) return false;
        Edge *e = &edges[i];
        *e = (Edge){in[0]*scale, in[1]*scale, in[2]*scale, in[3]*scale, in[4], in[5]};
        if (abs(e->ax) > 65536 || abs(e->az) > maxZ ||
            abs(e->bx) > 65536 || abs(e->bz) > maxZ ||
            abs(e->tx) > 4096 || abs(e->tz) > 4096 ||
            (e->ax == e->bx && e->az == e->bz))
            return false;
    }
    return true;
}

Floor collisionFloor(int x, int z)
{
    if (roomCurrent() == FirstFloor) {
        // SC0201 surfaces 8/9 lead upstairs, while 4/7 descend south of
        // the dividing wall. Both slopes use the original Q8 reference point.
        int localX = (x >> 4) - 20480, localZ = (z >> 4) - 10240;
        if (z >= 181248 && z <= 194832 && x >= 449328) {
            if (x >= 459040) return (Floor){-409*16, 0};
            return (Floor){(-205 + ((-2753*(localX-7906)) >> 12))*16, -2753};
        }
        if (z >= 162976 && z <= 180896 && x >= 449328) {
            if (x >= 459088) return (Floor){412*16, 0};
            int y = 204 + ((2778*(localX-7908)) >> 12) + ((5*(localZ-506)) >> 12);
            return (Floor){y*16, 2778};
        }
        return (Floor){-16, 0};
    }
    if (roomCurrent() != EastCorridor)
        return (Floor){roomCurrent() == SchoolCorridor ? 48 : 0, 0};
    // SCFE01 surfaces 11 and 12. The walls confine Harry to the stair
    // footprint. Evaluate the ramp in the same Q8 chunk space as the IPD.
    if (z >= 168000 && z <= 184720 && x >= -188240) return (Floor){6560, 0};
    if (z >= 168000 && z <= 181376 && x >= -198080 && x < -188240) {
        int y = 204 + ((2731*((x >> 4) + 20480 - 8407)) >> 12);
        return (Floor){y > 0 ? y*16 : 0, 2731};
    }
    return (Floor){0, 0};
}

static bool pushFromEdge(const Edge *e, int *x, int *z)
{
    int fromAX = *x-e->ax, fromAZ = *z-e->az;
    int fromBX = *x-e->bx, fromBZ = *z-e->bz;
    if ((fromAX > Radius && fromBX > Radius) || (fromAX < -Radius && fromBX < -Radius) ||
        (fromAZ > Radius && fromBZ > Radius) || (fromAZ < -Radius && fromBZ < -Radius))
        return false;

    int nx, nz, distance;
    if (fromBX*e->tx + fromBZ*e->tz < 0 || fromAX*e->tx + fromAZ*e->tz > 0) {
        // Beyond the segment, use its rounded end.
        int dx = fromBX, dz = fromBZ;
        if (fromAX*e->tx + fromAZ*e->tz > 0) {
            dx = fromAX;
            dz = fromAZ;
        }
        if (abs(dx) >= Radius || abs(dz) >= Radius) return false;
        distance = sqrt32(dx*dx + dz*dz);
        if (distance >= Radius || !distance) return false;
        nx = dx*4096/distance;
        nz = dz*4096/distance;
    } else {
        nx = -e->tz;
        nz = e->tx;
        int side = (fromBX*nx + fromBZ*nz) >> 12;
        distance = abs(side);
        if (distance >= Radius) return false;
        if (side < 0) { nx = -nx; nz = -nz; }
    }
    *x += (nx*(Radius-distance)) >> 12;
    *z += (nz*(Radius-distance)) >> 12;
    return true;
}

void collisionMove(int *worldX, int *worldZ, int dx, int dz)
{
    int x = *worldX - roomOriginX() + dx, z = *worldZ - roomOriginZ() + dz;
    // The largest walking step is under five centimetres. A second pass
    // resolves adjacent segments when sliding into a corner.
    for (unsigned pass = 0; pass < 2; pass++) {
        bool contact = false;
        for (unsigned i = 0; i < edgeCount; i++)
            contact |= pushFromEdge(&edges[i], &x, &z);
        if (!contact) break;
    }
    *worldX = x + roomOriginX();
    *worldZ = z + roomOriginZ();
}
