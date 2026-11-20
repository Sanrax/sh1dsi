// Local XZ collision on the school floors. Edges and tangent vectors
// come from the IPD splits used by the original character query.
#include <nds.h>
#include "collision.h"

typedef struct {
    int ax, az, bx, bz;
    int16_t tx, tz;
} Edge;
static Edge edges[128]; // Exterior boundaries and obstacles, plus door planes.
static unsigned edgeCount;
typedef struct { int x, z, radius; } ObstacleCylinder;
static ObstacleCylinder cylinders[15];
static unsigned cylinderCount;
typedef struct {
    struct { int16_t x, z; } corner[4];
    int16_t refX, height, refZ, slopeX, slopeZ;
} Ramp;
_Static_assert(sizeof(Ramp) == 26, "courtyard floor layout");
static Ramp courtyardRamps[14];
static unsigned courtyardRampCount;
enum { Radius = 1228 }; // Harry's 0.3-metre outer cylinder, Q12.

static int crossEdge(const Ramp *r, unsigned a, int x, int z)
{
    unsigned b = (a+1)&3;
    int ax = r->corner[a].x, az = r->corner[a].z;
    return (r->corner[b].x-ax)*(z-az) - (r->corner[b].z-az)*(x-ax);
}

static bool loadCourtyardFloors(FILE *file, bool towerFloors)
{
    if (towerFloors) {
        courtyardRampCount = sizeof courtyardRamps/sizeof courtyardRamps[0];
        if (fread(courtyardRamps, sizeof courtyardRamps, 1, file) != 1) return false;
    } else {
        // SHY1 stores only the rectangular or trapezoidal entrance walkways.
        struct { int16_t low0, high0, low1, high1, minZ, maxZ, y, z, slope; } old[4];
        if (fread(old, sizeof old, 1, file) != 1) return false;
        courtyardRampCount = sizeof old/sizeof old[0];
        for (unsigned i = 0; i < courtyardRampCount; i++) {
            courtyardRamps[i] = (Ramp){
                .corner = {{old[i].low0, old[i].minZ}, {old[i].high0, old[i].minZ},
                           {old[i].high1, old[i].maxZ}, {old[i].low1, old[i].maxZ}},
                .height = old[i].y, .refZ = old[i].z, .slopeZ = old[i].slope
            };
        }
    }
    for (unsigned i = 0; i < courtyardRampCount; i++) {
        const Ramp *r = &courtyardRamps[i];
        if (abs(r->refX) > 5120 || abs(r->refZ) > 5120 || abs(r->height) > 256 ||
            abs(r->slopeX) > 4096 || abs(r->slopeZ) > 4096) return false;
        for (unsigned j = 0; j < 4; j++) {
            unsigned k = (j+2)&3;
            if (abs(r->corner[j].x) > 5120 || abs(r->corner[j].z) > 5120 ||
                crossEdge(r, j, r->corner[k].x, r->corner[k].z) <= 0) return false;
        }
    }
    return true;
}

bool collisionLoad(FILE *file, SchoolRoom room, bool fullObstacles)
{
    cylinderCount = 0;
    switch (room) {
    case Courtyard: {
        uint16_t count;
        if (fread(&count, sizeof count, 1, file) != 1 || !count || count >= sizeof edges/sizeof edges[0])
            return false;
        edgeCount = count;
        break;
    }
    case Infirmary: edgeCount = 45; break;
    case FirstFloor: edgeCount = fullObstacles ? 51 : 39; break;
    case SouthHall: edgeCount = fullObstacles ? 47 : 43; break;
    case NorthHall: edgeCount = 24; break;
    case UpperNorthHall: edgeCount = 32; break;
    case ChemistryLab: edgeCount = 30; break;
    case EquipmentRoom: edgeCount = 32; break;
    case EastCorridor: edgeCount = 43; break;
    case SchoolCorridor: edgeCount = 20; break;
    case MusicRoom: edgeCount = 51; break;
    }
    int scale = room == MusicRoom ? 1 : 16;
    int originX = roomOriginX(room), originZ = roomOriginZ(room);
    // The first-floor south wall extends 0.21 m into the adjoining chunk.
    int maxZ = room == FirstFloor ? 21*4096 : 20*4096;
    int maxX = room == NorthHall ? 17*4096 : 16*4096;
    for (unsigned i = 0; i < edgeCount; i++) {
        int16_t in[6];
        if (fread(in, sizeof in, 1, file) != 1) return false;
        Edge *e = &edges[i];
        *e = (Edge){in[0]*scale, in[1]*scale, in[2]*scale, in[3]*scale, in[4], in[5]};
        if (abs(e->ax) > maxX || abs(e->az) > maxZ ||
            abs(e->bx) > maxX || abs(e->bz) > maxZ ||
            abs(e->tx) > 4096 || abs(e->tz) > 4096 ||
            (e->ax == e->bx && e->az == e->bz))
            return false;
        e->ax += originX;
        e->bx += originX;
        e->az += originZ;
        e->bz += originZ;
    }
    if (room == ChemistryLab) {
        cylinderCount = sizeof cylinders/sizeof cylinders[0];
        for (unsigned i = 0; i < cylinderCount; i++) {
            int16_t in[3];
            if (fread(in, sizeof in, 1, file) != 1 || abs(in[0]) > 5120 || abs(in[1]) > 5120 ||
                in[2] <= 0 || in[2] >= 256) return false;
            cylinders[i] = (ObstacleCylinder){originX+16*in[0], originZ+16*in[1], Radius+16*in[2]};
        }
    }
    if (room == Courtyard && !loadCourtyardFloors(file, fullObstacles)) return false;
    // Actor collision stops at the closed leaves. The facing ray can still
    // cross the threshold, and a room transition spawns beyond the radius.
    for (unsigned i = 0; i < schoolDoorCount; i++) {
        const Door *d = &schoolDoors[i];
        if (d->room != room) continue;
        if (edgeCount == sizeof edges/sizeof edges[0]) return false;
        int tx = d->nz, tz = -d->nx;
        int halfX = tx*d->halfWidth, halfZ = tz*d->halfWidth;
        edges[edgeCount++] = (Edge){d->x+halfX, d->z+halfZ, d->x-halfX, d->z-halfZ,
                                    tx*4096, tz*4096};
    }
    return true;
}

Floor collisionFloor(int x, int z)
{
    if (roomCurrent() == Courtyard) {
        int localX = (x-roomOriginX(Courtyard)) >> 4, localZ = (z-roomOriginZ(Courtyard)) >> 4;
        Floor floor = {0};
        for (unsigned i = 0; i < courtyardRampCount; i++) {
            const Ramp *r = &courtyardRamps[i];
            if (crossEdge(r, 0, localX, localZ) < 0 || crossEdge(r, 1, localX, localZ) < 0 ||
                crossEdge(r, 2, localX, localZ) < 0 || crossEdge(r, 3, localX, localZ) < 0) continue;
            int height = r->height + ((r->slopeX*(localX-r->refX)) >> 12) +
                         ((r->slopeZ*(localZ-r->refZ)) >> 12);
            if (height*16 < floor.height)
                floor = (Floor){.height = height*16, .slopeX = r->slopeX, .slopeZ = r->slopeZ};
        }
        return floor;
    }
    if (roomCurrent() == FirstFloor) {
        // SC0201 surfaces 8/9 lead upstairs, while 4/7 descend south of
        // the dividing wall. Both slopes use the original Q8 reference point.
        int localX = (x >> 4) - 20480, localZ = (z >> 4) - 10240;
        if (z >= 181248 && z <= 194832 && x >= 449328) {
            if (x >= 459040) return (Floor){.height = -409*16};
            return (Floor){.height = (-205 + ((-2753*(localX-7906)) >> 12))*16, .slopeX = -2753};
        }
        if (z >= 162976 && z <= 180896 && x >= 449328) {
            if (x >= 459088) return (Floor){.height = 412*16};
            int y = 204 + ((2778*(localX-7908)) >> 12) + ((5*(localZ-506)) >> 12);
            return (Floor){.height = y*16, .slopeX = 2778};
        }
        return (Floor){.height = -16};
    }
    if (roomCurrent() == Infirmary) return (Floor){.height = -48};
    if (roomCurrent() == NorthHall) return (Floor){.height = -16};
    if (roomCurrent() != EastCorridor)
        return (Floor){.height = roomCurrent() == SchoolCorridor ? 48 : 0};
    // SCFE01 surfaces 11 and 12. The walls confine Harry to the stair
    // footprint. Evaluate the ramp in the same Q8 chunk space as the IPD.
    if (z >= 168000 && z <= 184720 && x >= -188240) return (Floor){.height = 6560};
    if (z >= 168000 && z <= 181376 && x >= -198080 && x < -188240) {
        int y = 204 + ((2731*((x >> 4) + 20480 - 8407)) >> 12);
        return (Floor){.height = y > 0 ? y*16 : 0, .slopeX = 2731};
    }
    return (Floor){0};
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

static bool pushFromCylinder(const ObstacleCylinder *c, int *x, int *z)
{
    int dx = *x-c->x, dz = *z-c->z;
    if (abs(dx) >= c->radius || abs(dz) >= c->radius) return false;
    int distance = sqrt32(dx*dx + dz*dz);
    if (distance >= c->radius || !distance) return false;
    *x += dx*(c->radius-distance)/distance;
    *z += dz*(c->radius-distance)/distance;
    return true;
}

void collisionMove(int *worldX, int *worldZ, int dx, int dz)
{
    int x = *worldX + dx, z = *worldZ + dz;
    // The largest walking step is under five centimetres. A second pass
    // resolves adjacent segments when sliding into a corner.
    for (unsigned pass = 0; pass < 2; pass++) {
        bool contact = false;
        for (unsigned i = 0; i < edgeCount; i++)
            contact |= pushFromEdge(&edges[i], &x, &z);
        for (unsigned i = 0; i < cylinderCount; i++)
            contact |= pushFromCylinder(&cylinders[i], &x, &z);
        if (!contact) break;
    }
    *worldX = x;
    *worldZ = z;
}
