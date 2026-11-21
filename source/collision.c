// Local XZ collision on the school floors. Edges and tangent vectors
// come from the IPD splits used by the original character query.
#include <limits.h>
#include <nds.h>
#include "collision.h"

typedef struct {
    int ax, az, bx, bz;
    int16_t tx, tz, ay, by;
} Edge;
static Edge edges[128]; // Exterior boundaries and obstacles, plus door planes.
static unsigned edgeCount, heightEdgeCount;
typedef struct { int x, z, radius; } ObstacleCylinder;
static ObstacleCylinder cylinders[15];
static unsigned cylinderCount;
typedef struct { int16_t x, z; } FloorCorner;
typedef struct { int16_t refX, height, refZ, slopeX, slopeZ; } FloorEquation;
typedef struct {
    FloorCorner corner[8];
    FloorEquation equation;
    uint8_t corners;
} FloorPlane;
_Static_assert(sizeof(FloorCorner) == 4 && sizeof(FloorEquation) == 10, "floor plane layout");
static FloorPlane floorPlanes[14];
static unsigned floorPlaneCount;
static int floorMaxZ;
static bool groundPlanes;
static SchoolRoom floorPlaneRoom;
enum { Radius = 1228 }; // Harry's 0.3-metre outer cylinder, Q12.

static int crossEdge(FloorCorner a, FloorCorner b, int x, int z)
{
    return (b.x-a.x)*(z-a.z) - (b.z-a.z)*(x-a.x);
}

static bool loadFloorPlanes(FILE *file, FloorLayout layout, int maxZ)
{
    enum { Capacity = sizeof floorPlanes/sizeof floorPlanes[0] };
    bool counted = layout == CountedFloorPlanes || layout == CountedGroundPlanes;
    uint16_t count = 14; // SHY2 stores a fixed number of records.
    if (layout == LegacyWalkways) {
        // SHY1 stores only the rectangular or trapezoidal entrance walkways.
        struct { int16_t low0, high0, low1, high1, minZ, maxZ, y, z, slope; } old[4];
        if (fread(old, sizeof old, 1, file) != 1) return false;
        count = sizeof old/sizeof old[0];
        for (unsigned i = 0; i < count; i++) {
            floorPlanes[i] = (FloorPlane){
                .corner = {{old[i].low0, old[i].minZ}, {old[i].high0, old[i].minZ},
                           {old[i].high1, old[i].maxZ}, {old[i].low1, old[i].maxZ}},
                .equation = {.height = old[i].y, .refZ = old[i].z, .slopeZ = old[i].slope},
                .corners = 4
            };
        }
    } else {
        if (counted && fread(&count, sizeof count, 1, file) != 1) return false;
        if (!count || count > Capacity) return false;
        for (unsigned i = 0; i < count; i++) {
            FloorPlane *plane = &floorPlanes[i];
            plane->corners = 4;
            if (counted && fread(&plane->corners, 1, 1, file) != 1)
                return false;
            if (plane->corners < 3 || plane->corners > sizeof plane->corner/sizeof plane->corner[0] ||
                fread(plane->corner, sizeof(FloorCorner), plane->corners, file) != plane->corners ||
                fread(&plane->equation, sizeof(FloorEquation), 1, file) != 1)
                return false;
        }
    }
    for (unsigned i = 0; i < count; i++) {
        const FloorPlane *plane = &floorPlanes[i];
        const FloorEquation *eq = &plane->equation;
        if (abs(eq->refX) > 5120 || abs(eq->refZ) > maxZ ||
            (!counted && abs(eq->height) > 256) ||
            abs(eq->slopeX) > 4096 || abs(eq->slopeZ) > 4096) return false;
        for (unsigned j = 0; j < plane->corners; j++) {
            if (abs(plane->corner[j].x) > 5120 || abs(plane->corner[j].z) > maxZ)
                return false;
        }
        for (unsigned j = 0; j < plane->corners; j++) {
            const FloorCorner a = plane->corner[j];
            const unsigned end = (j+1 == plane->corners) ? 0 : j+1;
            for (unsigned k = 0; k < plane->corners; k++) {
                if (k != j && k != end &&
                    crossEdge(a, plane->corner[end], plane->corner[k].x, plane->corner[k].z) <= 0)
                    return false;
            }
        }
    }
    floorPlaneCount = count;
    floorMaxZ = maxZ;
    groundPlanes = layout == CountedGroundPlanes;
    return true;
}

static unsigned legacyEdgeCount(SchoolRoom room, EdgeLayout layout)
{
    switch (room) {
    case Infirmary: return 45;
    case FirstFloor: return layout == ExpandedEdges ? 51 : 39;
    case SouthHall: return layout == ExpandedEdges ? 47 : 43;
    case NorthHall: return 24;
    case UpperNorthHall: return 32;
    case ChemistryLab: return 30;
    case EquipmentRoom: return 32;
    case EastCorridor: return 43;
    case SchoolCorridor: return 20;
    case MusicRoom: return 51;
    default: return 0;
    }
}

bool collisionLoad(FILE *file, SchoolRoom room, EdgeLayout edgeLayout, FloorLayout floorLayout,
                   RoomOrigin origin)
{
    cylinderCount = 0;
    floorPlaneCount = 0;
    uint16_t count;
    if (edgeLayout == CountedEdges || edgeLayout == HeightEdges) {
        if (fread(&count, sizeof count, 1, file) != 1) return false;
    } else {
        count = legacyEdgeCount(room, edgeLayout);
    }
    if (!count || count >= sizeof edges/sizeof edges[0]) return false;
    edgeCount = count;
    heightEdgeCount = edgeLayout == HeightEdges ? count : 0;
    unsigned wordsPerEdge = heightEdgeCount ? 8 : 6;
    int scale = room == MusicRoom ? 1 : 16;
    // These streams include seams just beyond the chunk boundary.
    int maxZ = room == FirstFloor || room == OtherworldEastHall || room == OtherworldWestHall
             ? 21*4096 : 20*4096;
    int maxX = room == NorthHall || room == OtherworldNorthHall ? 17*4096 : 16*4096;
    for (unsigned i = 0; i < edgeCount; i++) {
        int16_t in[8] = {0};
        if (fread(in, sizeof in[0], wordsPerEdge, file) != wordsPerEdge) return false;
        Edge *e = &edges[i];
        *e = (Edge){in[0]*scale, in[1]*scale, in[2]*scale, in[3]*scale, in[4], in[5], in[6], in[7]};
        if (abs(e->ax) > maxX || abs(e->az) > maxZ ||
            abs(e->bx) > maxX || abs(e->bz) > maxZ ||
            abs(e->tx) > 4096 || abs(e->tz) > 4096 ||
            (e->ax == e->bx && e->az == e->bz))
            return false;
        if (heightEdgeCount && (e->ax-e->bx)*e->tx + (e->az-e->bz)*e->tz <= 0)
            return false;
        e->ax += origin.x;
        e->bx += origin.x;
        e->az += origin.z;
        e->bz += origin.z;
    }
    if (room == ChemistryLab) {
        cylinderCount = sizeof cylinders/sizeof cylinders[0];
        for (unsigned i = 0; i < cylinderCount; i++) {
            int16_t in[3];
            if (fread(in, sizeof in, 1, file) != 1 || abs(in[0]) > 5120 || abs(in[1]) > 5120 ||
                in[2] <= 0 || in[2] >= 256) return false;
            cylinders[i] = (ObstacleCylinder){origin.x+16*in[0], origin.z+16*in[1], Radius+16*in[2]};
        }
    }
    // Both Otherworld hall streams carry floor planes across their seams.
    int floorZ = room == OtherworldEastHall || room == OtherworldWestHall ? 21*256 : 20*256;
    if (floorLayout != NoFloorData && !loadFloorPlanes(file, floorLayout, floorZ)) return false;
    floorPlaneRoom = room;
    // Actor collision stops at the closed leaves. The facing ray can still
    // cross the threshold, and a room transition spawns beyond the radius.
    for (unsigned i = 0; i < schoolDoorCount; i++) {
        const Door *d = &schoolDoors[i];
        if (d->room != room) continue;
        if (edgeCount == sizeof edges/sizeof edges[0]) return false;
        int tx = d->nz, tz = -d->nx;
        int halfX = tx*d->halfWidth, halfZ = tz*d->halfWidth;
        edges[edgeCount++] = (Edge){d->x+halfX, d->z+halfZ, d->x-halfX, d->z-halfZ,
                                    tx*4096, tz*4096, 0, 0};
    }
    return true;
}

static bool floorContains(const FloorPlane *plane, int x, int z)
{
    FloorCorner previous = plane->corner[plane->corners-1];
    for (unsigned i = 0; i < plane->corners; i++) {
        FloorCorner next = plane->corner[i];
        if (crossEdge(previous, next, x, z) < 0) return false;
        previous = next;
    }
    return true;
}

Floor collisionFloor(int x, int z)
{
    SchoolRoom room = roomCurrent();
    if (floorPlaneCount && room == floorPlaneRoom) {
        RoomOrigin origin = roomOrigin();
        int localX = (x >> 4) - (origin.x >> 4), localZ = (z >> 4) - (origin.z >> 4);
        if (abs(localX) > 5120 || abs(localZ) > floorMaxZ) return (Floor){0};
        // Raised surfaces compete with the implicit y=0 ground. Authored
        // ground planes also cover floors below that datum (positive Y).
        Floor floor = {.height = groundPlanes ? INT_MAX : 0};
        for (unsigned i = 0; i < floorPlaneCount; i++) {
            const FloorPlane *plane = &floorPlanes[i];
            if (!floorContains(plane, localX, localZ)) continue;
            const FloorEquation *eq = &plane->equation;
            int height = eq->height + ((eq->slopeX*(localX-eq->refX)) >> 12) +
                         ((eq->slopeZ*(localZ-eq->refZ)) >> 12);
            if (height*16 < floor.height)
                floor = (Floor){.height = height*16, .slopeX = eq->slopeX, .slopeZ = eq->slopeZ};
        }
        return floor.height == INT_MAX ? (Floor){0} : floor;
    }
    if (room == FirstFloor) {
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
    if (room == Infirmary) return (Floor){.height = -48};
    if (room == NorthHall) return (Floor){.height = -16};
    if (room != EastCorridor)
        return (Floor){.height = room == SchoolCorridor ? 48 : 0};
    // SCFE01 surfaces 11 and 12. The walls confine Harry to the stair
    // footprint. Evaluate the ramp in the same Q8 chunk space as the IPD.
    if (z >= 168000 && z <= 184720 && x >= -188240) return (Floor){.height = 6560};
    if (z >= 168000 && z <= 181376 && x >= -198080 && x < -188240) {
        int y = 204 + ((2731*((x >> 4) + 20480 - 8407)) >> 12);
        return (Floor){.height = y > 0 ? y*16 : 0, .slopeX = 2731};
    }
    return (Floor){0};
}

static int edgeHeight(const Edge *e, int alongA, int alongB)
{
    if (e->ay == e->by || alongA >= 0) return e->ay;
    if (alongB <= 0) return e->by;
    // Projected distance from A and segment length, both in Q8.
    int distance = (-alongA) >> 16;
    int length = (alongB-alongA) >> 16;
    return length ? e->ay + (e->by-e->ay)*distance/length : e->ay;
}

static bool pushFromEdge(const Edge *e, int *x, int *z, int feet, bool hasHeight)
{
    int fromAX = *x-e->ax, fromAZ = *z-e->az;
    int fromBX = *x-e->bx, fromBZ = *z-e->bz;
    if ((fromAX > Radius && fromBX > Radius) || (fromAX < -Radius && fromBX < -Radius) ||
        (fromAZ > Radius && fromBZ > Radius) || (fromAZ < -Radius && fromBZ < -Radius))
        return false;

    int alongA = fromAX*e->tx + fromAZ*e->tz;
    int alongB = fromBX*e->tx + fromBZ*e->tz;
    int nx, nz, distance;
    if (alongB < 0 || alongA > 0) {
        // Beyond the segment, use its rounded end.
        int dx = fromBX, dz = fromBZ;
        if (alongA > 0) {
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
    // An edge below the actor's feet cannot push the ground collision circle.
    if (hasHeight && feet < edgeHeight(e, alongA, alongB)) return false;
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

void collisionMove(int *worldX, int *worldZ, int dx, int dz, int floorHeight)
{
    int x = *worldX + dx, z = *worldZ + dz;
    int feet = floorHeight >> 4;
    // The largest walking step is under five centimetres. A second pass
    // resolves adjacent segments when sliding into a corner.
    for (unsigned pass = 0; pass < 2; pass++) {
        bool contact = false;
        for (unsigned i = 0; i < edgeCount; i++)
            contact |= pushFromEdge(&edges[i], &x, &z, feet, i < heightEdgeCount);
        for (unsigned i = 0; i < cylinderCount; i++)
            contact |= pushFromCylinder(&cylinders[i], &x, &z);
        if (!contact) break;
    }
    *worldX = x;
    *worldZ = z;
}
