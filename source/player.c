// SPDX-License-Identifier: GPL-3.0-only
// Animation interpolation adapted from Anim_BonesUpdate.
// Copyright (C) 2026 shdecompilations
#include <nds.h>
#include "player.h"
#include "collision.h"
#include "room.h"
#include "statue.h"

typedef struct {
    int8_t parent, rotation, translation, offset[3];
} Bone;
typedef struct {
    int16_t xyz[3];
    uint16_t bone;
} Vertex;
typedef struct {
    uint16_t vertex;
    uint8_t u, v;
} Corner;
typedef struct {
    uint8_t palette, sides;
    uint16_t count;
} Batch;
typedef struct {
    int16_t r[3][3];
    int t[3];
} Matrix;

_Static_assert(sizeof(Bone) == 6 && sizeof(Vertex) == 8 &&
               sizeof(Corner) == 4 && sizeof(Batch) == 4, "player file layout");

static Bone bones[18];
enum { Idle, TurnLeft, TurnRight, WalkForward, WalkBackward, Kneel, Rise, Pour };
static const unsigned firstFrame[] = {0, 40, 53, 66, 89, 113, 128, 140};
static const unsigned frameCount[] = {40, 13, 13, 23, 24, 15, 12, 49};
static const unsigned frameRate[] = {10, 15, 15, 22, 23, 15, 15, 15};
static const unsigned blendRate[] = {5, 50, 50, 50, 20, 10, 10, 10};
static int8_t frames[189][156];
static Vertex vertices[384];
static Corner corners[1500];
static Batch batches[30];
static uint16_t vertexCount, batchCount;
static unsigned clip, frameQ12, blendQ12, blendFrom, tickFraction;
static int x = -409605, z = 81765, yaw = 1024;
static int speed;
static unsigned roomVersion;
static bool ready, halfFrame;

static int roadEyeX, roadEyeZ, entryX, entryZ;
static bool followingRoad, doorRail;
typedef struct {
    int x, y, z, watchX, watchY, watchZ;
} CameraPose;
typedef struct {
    SchoolRoom room, from;
    CameraPose camera;
} DoorView;
// Measured views at the courtyard and upstairs doors.
static const DoorView doorViews[] = {
    {Courtyard, SouthHall, {-80691, -10451, -280786, -80899, -6046, -258222}},
    {Courtyard, NorthHall, {-80691, -10451, -216468, -80449, -5852, -238997}},
    {NorthHall, Courtyard, {546816, -8003, 772695, 546561, -3404, 795226}},
    {UpperNorthHall, EastCorridor, {-62457, -7987, 784384, -84986, -3388, 784129}},
    {UpperNorthHall, ChemistryLab, {-118374, -7987, 789511, -118145, -3388, 766982}},
    {EastCorridor, UpperNorthHall, {-209115, -7987, 292864, -186584, -3388, 293117}},
    {EquipmentRoom, UpperNorthHall, {-402816, -8601, 413736, -403915, -2816, 401473}},
    {UpperNorthHall, EquipmentRoom, {-131481, -7987, 789512, -131252, -3388, 766982}}
};
static const CameraPose *doorView;
static bool eastLabRoad;
enum { EquipmentAisleSplit = -407552 };

static int limitToRoad(int value, int low, int high)
{
    return value < low ? low : value > high ? high : value;
}

static int limitChaseAxis(int value, int low, int high)
{
    // vcAdjustXzInLimAreaUsingMIN_IN_ROAD_DIST reserves a metre at each
    // side of the authored road. Narrow roads use their centre line.
    const int clearance = 4096;
    if (high-low < 2*clearance) return (low+high)/2;
    return limitToRoad(value, low+clearance, high-clearance);
}

enum { MaxChaseClearance = 4*4096 };

// vcFlipFromCamExclusionArea uses a radius of 1.3 m behind Harry and
// up to 4 m in front. Interpolate against the cosine to stay in Q12.
static bool insideChaseClearance(int dx, int dz, int facingX, int facingZ)
{
    static const int cosine[] = {4096, 3784, 2896, 1567, 0, -1567, -2896, -3784, -4096};
    static const int radius[] = {16384, 16384, 15155, 13107, 11059, 9420, 7782, 6144, 5324};
    if (abs(dx) >= MaxChaseClearance || abs(dz) >= MaxChaseClearance) return false;
    int distance = sqrt32(dx*dx + dz*dz);
    if (!distance) return true;
    int dot = limitToRoad((dx*facingX + dz*facingZ)/distance, -4096, 4096);
    unsigned span = 1;
    while (dot < cosine[span]) span++;
    int clearance = radius[span] + (radius[span-1]-radius[span])*(dot-cosine[span]) /
                    (cosine[span-1]-cosine[span]);
    return distance < clearance;
}

static int chaseAxisOffset(int along, int across, int previous, int facing, int crossFacing)
{
    // Keep the current side of Harry if a straight crossing would pass
    // inside the exclusion radius. Search along the length of the road.
    bool lower = along < 0;
    if (lower != (previous < 0) && insideChaseClearance(0, across, facing, crossFacing)) {
        lower = previous < 0;
        along = 0;
    }
    if (!insideChaseClearance(along, across, facing, crossFacing)) return along;

    int minDistance = abs(along), maxDistance = MaxChaseClearance;
    while (maxDistance-minDistance > 64) {
        int trial = (minDistance+maxDistance)/2;
        if (insideChaseClearance(lower ? -trial : trial, across, facing, crossFacing))
            minDistance = trial;
        else
            maxDistance = trial;
    }
    return lower ? -maxDistance : maxDistance;
}

// MAP1_S00 camera_paths: two fixed positions and a higher priority road by
// the cabinet. Horizontal bounds and camera coordinates use the PS1 Q4 units.
typedef struct {
    int16_t lowX, highX, lowZ, highZ, eyeX, eyeZ, eyeY, watchY;
} InfirmaryRoad;
static const InfirmaryRoad infirmaryRoads[] = {
    {896, 960, 2208, 2304, 984, 2256, -24, -23},
    {960, 1008, 2208, 2304, 955, 2281, -43, -20},
    {976, 1008, 2281, 2304, 968, 2265, -43, -20}
};
static unsigned infirmaryRoad;

static int roadDistance(const InfirmaryRoad *r)
{
    int dx = x - limitToRoad(x, r->lowX*256, r->highX*256);
    int dz = z - limitToRoad(z, r->lowZ*256, r->highZ*256);
    return abs(dx) + abs(dz);
}

static void updateInfirmaryCamera(void)
{
    // vcSetCurNearRoadInVC_WORK gives the current road a 0.35-metre
    // advantage and holds an effect road until twice that distance outside.
    int distances[3];
    for (unsigned i = 0; i < 3; i++) distances[i] = roadDistance(&infirmaryRoads[i]);
    unsigned next = distances[0] < distances[1] ? 0 : 1;
    if (!distances[2]) infirmaryRoad = 2;
    else if (infirmaryRoad == 2 && distances[2] < 2*1433) return;
    else if (distances[next] < distances[infirmaryRoad]-1433) infirmaryRoad = next;
}

static void advanceRoadCamera(void)
{
    if (roomCurrent() == Infirmary) {
        updateInfirmaryCamera();
        return;
    }
    if (roomCurrent() == ChemistryLab) {
        // MAP1_S01 switches between two stationary chase roads in the lab.
        int split = -246784;
        eastLabRoad = x > split + (eastLabRoad ? -1433 : 1433);
        return;
    }
    if (roomCurrent() == MusicRoom) return;
    // Leave the entry view as Harry walks out along the road. The small
    // chase road places the ideal eye two metres away, twelve degrees to
    // one side of the heading opposite Harry (vcMakeIdealCamPosUseVC_ROAD_DATA).
    if (abs(x-entryX) > 8192 || abs(z-entryZ) > 8192 ||
        (roomCurrent() == EquipmentRoom && x < EquipmentAisleSplit)) followingRoad = true;
    if (!followingRoad) return;
    int angle = (yaw + 2048 - 136)*8;
    int sx = x + 2*sinLerp(angle), sz = z + 2*cosLerp(angle);
    if (roomCurrent() == SchoolCorridor) {
        sx = limitChaseAxis(sx, -121088, -42496);
        sz = limitChaseAxis(sz, 75264, 88320);
    } else if (roomCurrent() == SouthHall) {
        sx = limitChaseAxis(sx, 534016, 612608);
        sz = limitChaseAxis(sz, 75264, 86272);
        int offset = chaseAxisOffset(sx-x, sz-z, roadEyeX-x, sinLerp(yaw*8), cosLerp(yaw*8));
        sx = limitChaseAxis(x+offset, 534016, 612608);
    } else if (roomCurrent() == NorthHall) {
        sx = limitChaseAxis(sx, 507392, 585984);
        sz = limitChaseAxis(sz, 779008, 790784);
    } else if (roomCurrent() == UpperNorthHall) {
        sx = limitChaseAxis(sx, -139520, -60928);
        sz = limitChaseAxis(sz, 777728, 788736);
    } else if (roomCurrent() == EquipmentRoom) {
        sx = limitChaseAxis(sx, -404736, -400896);
        sz = limitChaseAxis(sz, 399360, 420096);
    } else if (roomCurrent() == Courtyard) {
        // MAP1_S00 courtyard chase road bounds.
        sx = limitChaseAxis(sx, -111616, -50176);
        sz = limitChaseAxis(sz, -280576, -217088);
    } else if (roomCurrent() == FirstFloor) {
        sx = limitChaseAxis(sx, 437760, 449536);
        sz = limitChaseAxis(sz, 174080, 299264);
        int offset = chaseAxisOffset(sz-z, sx-x, roadEyeZ-z, cosLerp(yaw*8), sinLerp(yaw*8));
        sz = limitChaseAxis(z+offset, 174080, 299264);
    } else {
        sx = limitChaseAxis(sx, -208384, -197376);
        sz = limitChaseAxis(sz, 175104, 299008);
    }
    roadEyeX += (sx-roadEyeX)/10;
    roadEyeZ += (sz-roadEyeZ)/10;
}

bool playerReady(void)
{
    return ready;
}

bool playerLoad(FILE *file, unsigned version)
{
    roomVersion = version;
    if (fread(&vertexCount, 2, 1, file) != 1 || vertexCount > 384 || !vertexCount ||
        fread(&batchCount, 2, 1, file) != 1 || batchCount > 30 || !batchCount ||
        fread(bones, sizeof bones, 1, file) != 1)
        return false;
    uint16_t clips = 1;
    if (version >= 3 && (fread(&clips, 2, 1, file) != 1 || clips != (version == 5 ? 7 : version == 4 ? 5 : 3)))
        return false;
    unsigned count = firstFrame[clips-1] + frameCount[clips-1];
    if (fread(frames, sizeof frames[0], count, file) != count ||
        fread(vertices, sizeof(Vertex), vertexCount, file) != vertexCount)
        return false;
    for (int i = 1; i < 18; i++) {
        const Bone *b = &bones[i];
        if (b->parent < 0 || b->parent >= i || b->rotation != i-1 ||
            b->translation != ((i == 1 || i == 11) ? 0 : -1))
            return false;
    }
    for (unsigned i = 0; i < vertexCount; i++) {
        if (vertices[i].bone >= 18)
            return false;
    }
    unsigned next = 0;
    for (unsigned i = 0; i < batchCount; i++) {
        Batch *b = &batches[i];
        if (fread(b, sizeof *b, 1, file) != 1 || b->palette >= 15 ||
            (b->sides != 3 && b->sides != 4) || b->count % b->sides ||
            b->count > 1500-next || fread(corners+next, sizeof(Corner), b->count, file) != b->count)
            return false;
        next += b->count;
    }
    for (unsigned i = 0; i < next; i++) {
        if (corners[i].vertex >= vertexCount)
            return false;
    }
    ready = true;
    return true;
}

// Digital, unarmed Player_LowerBodyUpdate on a level floor.
static unsigned locomotion(int turn, int walk, int dt)
{
    int direction = clip == WalkForward ? 1 : clip == WalkBackward ? -1 : 0;
    int acceleration = dt * 1638 / 136;
    if (direction && walk == direction) {
        int limit = direction > 0 ? 5734 : 4710;
        if (!speed || direction < 0 || frameQ12 >= 4096)
            speed += acceleration;
        if (speed > limit) speed = limit;
    } else if (speed) {
        speed -= 2*acceleration;
        if (speed < 0) speed = 0;
    }
    if (speed) walk = direction;
    int turnRate = walk > 0 ? 5 : walk < 0 ? -4 : 7;
    yaw = (yaw + ((turn * turnRate * dt) >> 4)) & 4095;
    if (speed) {
        int heading = yaw*8 + (walk < 0 ? 16384 : 0);
        int sx = sinLerp(heading), sz = cosLerp(heading);
        int vx = (speed*sx) >> 12, vz = (speed*sz) >> 12;
        Floor floor = collisionFloor(x, z);
        if (floor.slopeX) {
            // Player_Control applies cos(gradient/8) twice before computing speed.
            int alpha = cosLerp((floor.slopeX/8)*8);
            vx = (((vx*alpha) >> 12)*alpha) >> 12;
        }
        if (floor.slopeZ) {
            int alpha = cosLerp((floor.slopeZ/8)*8);
            vz = (((vz*alpha) >> 12)*alpha) >> 12;
        }
        unsigned square = vx*vx + vz*vz;
        // SquareRoot0 indexes its table with a normalized eight-bit value.
        // Preserve that quantization when using the ARM9 square-root unit.
        if (square >= 256) {
            unsigned shift = 24 - (__builtin_clz(square) & ~1u);
            square = (square >> shift) << shift;
        }
        int distance = ((int)sqrt32(square)*dt) >> 12;
        collisionMove(&x, &z, (sx*distance) >> 12, (sz*distance) >> 12);
    }
    if (walk) return walk > 0 ? WalkForward : WalkBackward;
    return turn < 0 ? TurnLeft : turn > 0 ? TurnRight : Idle;
}

bool playerStopped(void)
{
    return speed == 0;
}

bool playerLoadPour(FILE *file)
{
    return fread(frames+firstFrame[Pour], sizeof frames[0], frameCount[Pour], file) == frameCount[Pour];
}

bool playerActionDone(PlayerAction action)
{
    return clip == (action == PlayerKneel ? Kneel : Rise) &&
           frameQ12 == (frameCount[clip]-1)*4096;
}

bool playerTick(int turn, int walk, PlayerAction action)
{
    if (!ready) return false;
    halfFrame = !halfFrame;
    if (halfFrame) return false;
    tickFraction += 33366 * 4096;
    int dt = tickFraction / 1000000;
    tickFraction %= 1000000;
    unsigned next = action == PlayerMove ?
        locomotion(roomVersion >= 3 ? turn : 0, roomVersion >= 4 ? walk : 0, dt) :
        action == PlayerKneel ? Kneel : action == PlayerRise ? Rise : Pour;
    advanceRoadCamera();
    if (next != clip) {
        blendFrom = firstFrame[clip] + (frameQ12 >> 12);
        clip = next;
        frameQ12 = 0;
        blendQ12 = 4096;
    }
    if (blendQ12) {
        unsigned advance = dt * blendRate[clip];
        blendQ12 = advance < blendQ12 ? blendQ12-advance : 0;
    } else {
        frameQ12 += dt * frameRate[clip];
        if (clip < Kneel)
            frameQ12 %= frameCount[clip]*4096;
        else if (frameQ12 >= (frameCount[clip]-1)*4096)
            frameQ12 = (frameCount[clip]-1)*4096;
    }
    return true;
}

static void warp(int worldX, int worldZ, int heading)
{
    x = worldX;
    z = worldZ;
    yaw = heading;
    speed = 0;
    clip = Idle;
    frameQ12 = blendQ12 = 0;
}

void playerWarpForMedal(void)
{
    warp(-100*4096, 82329, 3840);
}

void playerWarpForPour(void)
{
    warp(-242483, 405094, 0);
}

void playerEnterRoom(SchoolRoom from)
{
    doorView = NULL;
    bool downstairs = roomCurrent() == FirstFloor;
    if ((downstairs && from == EastCorridor) || (from == FirstFloor && roomCurrent() == EastCorridor)) {
        // A LoadOverlay warp uses the map point directly, without the
        // 0.4-metre offset applied to ordinary doors.
        warp(downstairs ? 454656 : -192512, downstairs ? 188006 : 174899, 3072);
        entryX = x;
        entryZ = z;
        followingRoad = doorRail = false;
        roadEyeX = downstairs ? 441876 : -204178;
        roadEyeZ = downstairs ? 179657 : 184306;
        return;
    }
    for (unsigned i = 0; i < schoolDoorCount; i++) {
        const Door *d = &schoolDoors[i];
        if (d->room == roomCurrent() && d->other == from) {
            int heading = d->nx ? (d->nx > 0 ? 1024 : 3072) : (d->nz > 0 ? 0 : 2048);
            warp(d->x + 1638*d->nx, d->z + 1638*d->nz, heading);
            entryX = x;
            entryZ = z;
            followingRoad = false;
            for (unsigned j = 0; j < sizeof doorViews/sizeof doorViews[0]; j++) {
                const DoorView *view = &doorViews[j];
                if (view->room != d->room || view->from != from) continue;
                doorView = &view->camera;
                roadEyeX = doorView->x;
                roadEyeZ = doorView->z;
                doorRail = true;
                return;
            }
            doorRail = from == EastCorridor || d->room == EastCorridor;
            // Camera entry anchors for MAP_POINTS 39, 51 and 36.
            roadEyeX = !doorRail ? -116992 : d->room == EastCorridor ? -209108 : -41644;
            roadEyeZ = !doorRail ? 83807 : z;
            if (from == Courtyard) {
                doorRail = true;
                roadEyeX = x;
                roadEyeZ = z+206;
            } else if (d->room == ChemistryLab) {
                eastLabRoad = false;
            } else if (d->room == Infirmary) {
                infirmaryRoad = 0;
            } else if (from == Infirmary) {
                doorRail = true;
                roadEyeX = x;
                roadEyeZ = z-212;
            } else if (d->room == SouthHall || from == SouthHall) {
                doorRail = true;
                roadEyeX = d->room == SouthHall ? 611329 : x-212;
                roadEyeZ = z;
            }
            return;
        }
    }
}

SchoolRoom playerDestination(bool action)
{
    SchoolRoom room = roomCurrent();
    // Event_Collide: MAP1_S01 point 80 / MAP1_S00 point 79 use a
    // TouchAabb with both half-extents set to 6 quarter-metres.
    if (room == EastCorridor && abs(x + 184320) <= 6144 && abs(z - 174899) <= 6144)
        return FirstFloor;
    if (room == FirstFloor && abs(x - 462848) <= 6144 && abs(z - 188006) <= 6144)
        return EastCorridor;
    if (!action || roomVersion < 5) return room;
    int rayX = sinLerp(yaw*8)/2, rayZ = cosLerp(yaw*8)/2;
    for (unsigned i = 0; i < schoolDoorCount; i++) {
        const Door *d = &schoolDoors[i];
        if (d->room != room) continue;
        int dx = d->x-x, dz = d->z-z;
        int along = -dx*d->nx - dz*d->nz;
        int reach = -rayX*d->nx - rayZ*d->nz;
        if (reach > 0 && along >= 0 && along <= reach &&
            abs(rayX*dz - dx*rayZ) <= d->halfWidth*reach)
            return d->other;
    }
    return room;
}

static bool facingEdge(int edgeX, int edgeZ, int halfWidth, int nx, int nz)
{
    // Event_CollideObbFacingCheck intersects a half-metre facing ray
    // with an axis-aligned interaction edge.
    int dx = edgeX - x, dz = edgeZ - z;
    int rayX = sinLerp(yaw*8)/2, rayZ = cosLerp(yaw*8)/2;
    int reach = rayX*nx + rayZ*nz, distance = dx*nx + dz*nz;
    if (reach <= 0 || distance < 0 || distance > reach)
        return false;
    int along = rayZ*dx - dz*rayX;
    return abs(along) <= halfWidth * reach;
}

bool playerFacingPiano(void)
{
    return roomVersion < 3 || facingEdge(-408371, 81510, 3072, 1, 0);
}

bool playerFacingNotebook(void)
{
    // MAP1_S00 point 89, facing east, triggerParam1 = 4 sixteenths of a metre.
    return roomCurrent() == Infirmary && facingEdge(252723, 575897, 1024, 1, 0);
}

bool playerFacingChemical(void)
{
    // MAP1_S01 point 13 faces west across a 0.75-metre shelf segment.
    return facingEdge(-413696, 411238, 1536, -1, 0);
}

bool playerFacingHand(void)
{
    // MAP1_S01 point 12 faces north across the lab bench.
    return roomHasHand() && facingEdge(-242483, 406323, 1536, 0, 1);
}

TowerPoint playerFacingTower(void)
{
    if (roomCurrent() != Courtyard) return NoTowerPoint;
    // MAP1_S00 points 13–15. A half-metre ray intersects the oriented
    // segment, from the same side as Event_CollideObbFacingCheck.
    static const struct { int x, z, nx, nz, halfWidth; } points[] = {
        {-53657, -279756, -2896, 2896, 2048},
        {-49971, -277299, 0, 4096, 1536},
        {-56115, -283033, -4096, 0, 1536}
    };
    int rx = sinLerp(yaw*8)/2, rz = cosLerp(yaw*8)/2;
    for (unsigned i = 0; i < sizeof points/sizeof points[0]; i++) {
        int dx = points[i].x-x, dz = points[i].z-z;
        if (abs(dx) > 4096 || abs(dz) > 4096) continue;
        int distance = -dx*points[i].nx - dz*points[i].nz;
        int reach = -rx*points[i].nx - rz*points[i].nz;
        if (reach > 0 && distance >= 0 && distance <= reach &&
            (int64_t)abs(rx*dz-dx*rz)*4096 <= (int64_t)points[i].halfWidth*reach)
            return TowerDoor+i;
    }
    return NoTowerPoint;
}

void playerSavePose(Checkpoint *point)
{
    point->room = roomCurrent();
    point->x = x;
    point->z = z;
    point->yaw = yaw;
}

void playerRestorePose(const Checkpoint *point)
{
    warp(point->x, point->z, point->yaw);
    infirmaryRoad = 0;
    updateInfirmaryCamera();
}

bool playerFacingPoint(int worldX, int worldZ)
{
    // Event_CollideFacingCheck puts the origin of the 0.8-metre sector
    // one eighth of a metre behind Harry.
    int sx = sinLerp(yaw*8), sz = cosLerp(yaw*8);
    int dx = worldX - x + sx/8, dz = worldZ - z + sz/8;
    if (abs(dx) > 3276 || abs(dz) > 3276 || dx*dx+dz*dz > 3276*3276)
        return false;
    int along = dx*sx + dz*sz;
    // tan(30 degrees) in Q12. The cross product is the lateral component.
    return along > 0 && abs(dx*sz-dz*sx) <= (int)(((int64_t)along*2365) >> 12);
}

static int setCameraPose(const CameraPose *view, int origin, int originZ)
{
    int height = -view->y/2;
    gluLookAtf32((view->x-origin)/2, height, (originZ-view->z)/2,
                 (view->watchX-origin)/2, -view->watchY/2,
                 (originZ-view->watchZ)/2, 0,4096,0);
    return height;
}

int playerSetCamera(void)
{
    // vcMakeNormalWatchTgtPos / vcSetWatchTgtXzPos, small chase road.
    // Its one-metre watch circle extends away from the room camera whenever
    // the distance to Harry is below four metres.
    SchoolRoom room = roomCurrent();
    bool corridor = room == SchoolCorridor || room == SouthHall || room == NorthHall || room == UpperNorthHall;
    bool equipment = room == EquipmentRoom;
    bool stairwell = room == EastCorridor || room == FirstFloor;
    bool infirmary = room == Infirmary;
    bool courtyard = room == Courtyard;
    bool lab = room == ChemistryLab;
    int ground = collisionFloor(x, z).height;
    int origin = roomOriginX(room), originZ = roomOriginZ(room);
    if (lab && statueCamera()) {
        static const CameraPose pour = {-244162, -1679, 399278, -240025, -9379, 413163};
        return setCameraPose(&pour, origin, originZ);
    }
    if (doorView && !followingRoad) return setCameraPose(doorView, origin, originZ);
    if (equipment && x < EquipmentAisleSplit) {
        // The two fixed-angle roads overlap near the middle of the west aisle.
        static const CameraPose westAisle[] = {
            {-409856, -11059, 415744, -415184, -6691, 400864},
            {-409856, -9622, 403712, -413552, -870, 417040}
        };
        return setCameraPose(&westAisle[z >= 409600], origin, originZ);
    }
    if (room == MusicRoom && x >= -419840 && x <= -416768 && z >= 94976 && z <= 98304) {
        // Fixed-angle road looking back through the music-room doorway.
        gluLookAtf32((-420864-origin)/2, 768, (81920-82688)/2,
                     (-414400-origin)/2, 3904, (81920-96368)/2, 0,4096,0);
        return 768;
    }
    int eyeX = -417792, eyeZ = 74752, eyeHeight = 6144, targetHeight = -1024;
    if (infirmary) {
        const InfirmaryRoad *r = &infirmaryRoads[infirmaryRoad];
        eyeX = r->eyeX*256;
        eyeZ = r->eyeZ*256;
        eyeHeight = -r->eyeY*128;
        targetHeight = (-r->watchY*256-ground)/2;
    } else if (lab) {
        eyeX = eastLabRoad ? -251392 : -254720;
        eyeZ = eastLabRoad ? 402176 : 419328;
        eyeHeight = 4608;
        targetHeight = 1408;
    } else if (corridor || stairwell || courtyard || equipment) {
        eyeX = roadEyeX;
        eyeZ = roadEyeZ;
        bool rail = doorRail && !followingRoad;
        // The authored southern hall road fixes the chase eye at Q4 -27.
        const int chaseHeight = room == SouthHall ? 27*256 : 8600;
        eyeHeight = ((rail ? 7987 : chaseHeight)-ground)/2;
        targetHeight = ((rail ? 3533 : 2816)-ground)/2;
        // The small road limits the camera to 2.5 metres above map origin.
        if (eyeHeight > 5120) eyeHeight = 5120;
    }
    int dx = x-eyeX, dz = z-eyeZ;
    int length = sqrt32(dx*dx + dz*dz);
    int extra = length < 4*4096 ? 4*4096-length : 0;
    int targetX = x + sinLerp(yaw*8), targetZ = z + cosLerp(yaw*8);
    if (length) {
        targetX += dx * extra / length;
        targetZ += dz * extra / length;
    }
    if ((corridor || stairwell || infirmary || courtyard || lab || equipment) && length && !(doorRail && !followingRoad)) {
        // vcAdjCamOfsAngByCharaInScreen keeps Harry's feet inside the
        // vertical half-angle atan(224/224)/2. Work in slopes to avoid
        // converting the ray to Euler angles and back.
        int height = eyeHeight*2+ground;
        int watchX = targetX-eyeX, watchZ = targetZ-eyeZ;
        int watchDistance = sqrt32(watchX*watchX + watchZ*watchZ);
        int drop = (int)((int64_t)watchDistance*(height-(length*1697 >> 12)) /
                         (length+(height*1697 >> 12)));
        int limit = eyeHeight-drop/2;
        if (targetHeight > limit) targetHeight = limit;
    }
    gluLookAtf32((eyeX-origin)/2, eyeHeight, (originZ-eyeZ)/2,
                 (targetX-origin)/2, targetHeight, (originZ-targetZ)/2, 0, 4096, 0);
    return eyeHeight;
}

static int dot(const int16_t a[3], const int16_t b[3])
{
    return (a[0]*b[0] + a[1]*b[1] + a[2]*b[2]) >> 12;
}

static void skeleton(Matrix world[18])
{
    unsigned index = frameQ12 >> 12;
    int alpha = frameQ12 & 4095;
    const int8_t *a = frames[firstFrame[clip]+index];
    const int8_t *b = frames[firstFrame[clip]+(index+1)%frameCount[clip]];
    if (blendQ12) {
        a = frames[blendFrom];
        b = frames[firstFrame[clip]];
        alpha = 4096-blendQ12;
    }
    world[0] = (Matrix){.r = {{4096,0,0}, {0,4096,0}, {0,0,4096}}};
    for (int i = 1; i < 18; i++) {
        const Bone *bone = &bones[i];
        const Matrix *parent = &world[(unsigned)bone->parent];
        int16_t columns[3][3], t[3];
        for (unsigned j = 0; j < 3; j++) {
            // Both hips and torso use translation channel zero for this clip.
            t[j] = bone->translation == 0 ? 4*a[j] + (((b[j]-a[j])*alpha) >> 10)
                                              - (j == 1 ? 255 : 0) : 4*bone->offset[j];
            for (unsigned k = 0; k < 3; k++) {
                unsigned n = 3 + 9*bone->rotation + 3*k + j;
                columns[j][k] = 32*a[n] + (((b[n]-a[n])*alpha) >> 7);
            }
        }
        for (unsigned j = 0; j < 3; j++) {
            world[i].t[j] = parent->t[j] + dot(parent->r[j], t);
            for (unsigned k = 0; k < 3; k++)
                world[i].r[j][k] = dot(parent->r[j], columns[k]);
        }
    }
}

void playerDraw(unsigned firstPalette)
{
    Matrix world[18];
    static int16_t positions[384][3];
    skeleton(world);
    for (unsigned i = 0; i < vertexCount; i++) {
        const Vertex *v = &vertices[i];
        const Matrix *m = &world[v->bone];
        for (unsigned j = 0; j < 3; j++)
            positions[i][j] = (m->t[j] + dot(m->r[j], v->xyz)) * (j ? -8 : 8);
    }

    glPushMatrix();
    // PS1 bone coordinates are in Q8. Convert root pose to room-local DS units.
    SchoolRoom room = roomCurrent();
    glTranslatef32(((x-roomOriginX(room)) >> 4)*8, -collisionFloor(x, z).height/2, ((roomOriginZ(room) >> 4)-(z >> 4))*8);
    glRotateYi(-yaw*8);
    glPolyFmt(POLY_ALPHA(31) | POLY_CULL_FRONT);
    const Corner *c = corners;
    for (unsigned i = 0; i < batchCount; i++) {
        const Batch *b = &batches[i];
        GFX_PAL_FORMAT = 2*(firstPalette + b->palette);
        glBegin(b->sides == 3 ? GL_TRIANGLES : GL_QUADS);
        for (unsigned j = 0; j < b->count; j++, c++) {
            const int16_t *v = positions[c->vertex];
            glTexCoord2t16(c->u << 4, c->v << 4);
            glVertex3v16(v[0], v[1], v[2]);
        }
        glEnd();
    }
    glPopMatrix(1);
}
