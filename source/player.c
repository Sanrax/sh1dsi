// SPDX-License-Identifier: GPL-3.0-only
// Animation interpolation adapted from Anim_BonesUpdate.
// Copyright (C) 2026 shdecompilations
#include <nds.h>
#include "player.h"
#include "collision.h"
#include "room.h"
#include "statue.h"
#include "otherworld.h"

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
    uint32_t commands;
    uint32_t parameters[2][3];
} VertexPair;
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
enum { Idle, TurnLeft, TurnRight, WalkForward, WalkBackward, Kneel, Rise, Pour, DescendLadder, AscendLadder, LookAround };
static const unsigned firstFrame[] = {0, 40, 53, 66, 89, 113, 128, 140, 189, 226, 263};
static const unsigned frameCount[] = {40, 13, 13, 23, 24, 15, 12, 49, 37, 37, 47};
static const unsigned frameRate[] = {10, 15, 15, 22, 23, 15, 15, 15, 18, 18, 9};
static const unsigned blendRate[] = {5, 50, 50, 50, 20, 10, 10, 10, 10, 10, 10};
static int8_t frames[310][156];
static int ladderAge = -1;
static int ladderHeight;
static SchoolRoom ladderTower;
static bool ladderAscending, ladderAscentReady;
enum { LadderAscentMotion = 51 };
static Vertex vertices[384];
static Corner corners[1500];
static Batch batches[30];
static VertexPair *pairs;
static uint16_t vertexCount, batchCount;
static unsigned clip, frameQ12, blendQ12, blendFrom, tickFraction;
static int x = -409605, z = 81765, yaw = 1024;
static int speed;
static unsigned roomVersion;
static bool ready, halfFrame;

static int roadEyeX, roadEyeZ, entryX, entryZ;
static bool followingRoad, doorRail;
typedef struct {
    SchoolRoom room, from;
    CameraPose camera;
    int doorZ; // Select the entry when two doors connect the same rooms.
} DoorView;
// Measured views at the door and stair arrivals.
static const DoorView doorViews[] = {
    {Courtyard, SouthHall, {-80691, -10451, -280786, -80899, -6046, -258222}, 0},
    {Courtyard, NorthHall, {-80691, -10451, -216468, -80449, -5852, -238997}, 0},
    {NorthHall, Courtyard, {546816, -8003, 772695, 546561, -3404, 795226}, 0},
    {OtherworldCourtyard, OtherworldNorthHall, {-80691, -10451, -216468, -80438, -5852, -238997}, 0},
    {OtherworldNorthHall, OtherworldCourtyard, {546816, -8003, 772695, 546561, -3404, 795226}, 0},
    {OtherworldLobby, OtherworldNorthHall, {409600, -7987, 561752, 409356, -3388, 584283}, 0},
    {OtherworldNorthHall, OtherworldLobby, {546816, -8003, 789510, 547058, -3404, 766981}, 0},
    {OtherworldSideHall, OtherworldNorthHall, {543744, -8601, 576307, 543976, -2816, 564018}, 0},
    {OtherworldNorthHall, OtherworldSideHall, {564838, -8003, 789510, 565091, -3404, 766981}, 0},
    {OtherworldEastHall, OtherworldSideHall, {441548, -8003, 297991, 441777, -3404, 275462}, 0},
    {OtherworldSideHall, OtherworldEastHall, {560912, -8601, 572802, 571566, -2816, 564423}, 0},
    {UpperNorthHall, EastCorridor, {-62457, -7987, 784384, -84986, -3388, 784129}, 0},
    {UpperNorthHall, ChemistryLab, {-118374, -7987, 789511, -118145, -3388, 766982}, 0},
    {EastCorridor, UpperNorthHall, {-209115, -7987, 292864, -186584, -3388, 293117}, 0},
    {EquipmentRoom, UpperNorthHall, {-402816, -8601, 413736, -403915, -2816, 401473}, 0},
    {UpperNorthHall, EquipmentRoom, {-131481, -7987, 789512, -131252, -3388, 766982}, 0},
    {BasementStair, FirstFloor, {224047, -10240, -85118, 234373, -10000, -77339}, 0},
    {BoilerRoom, BasementStair, {88832, -6921, -68113, 88695, -2816, -84057}, 0},
    {BasementStair, BoilerRoom, {220964, -7987, -70451, 243480, -3687, -70198}, 0},
    {FirstFloor, BasementStair, {442003, -5049, 182974, 451525, 736, 173841}, 0},
    {NorthClassroom, OtherworldEastHall, {567327, -10982, 420909, 560623, -4096, 433260}, 431718},
    {NorthClassroom, OtherworldEastHall, {572418, -10944, 393185, 561236, -4096, 387192}, 387481},
    {OtherworldEastHall, NorthClassroom, {448313, -8003, 294502, 425784, -3404, 294247}, 294502},
    {OtherworldEastHall, NorthClassroom, {448313, -8003, 251904, 425784, -3404, 251649}, 251904},
    {SouthClassroom, NorthClassroom, {581555, -10993, 255217, 586700, -4096, 266703}, 0},
    {NorthClassroom, SouthClassroom, {570480, -10941, 398047, 578974, -4096, 388095}, 0},
    {OtherworldEastHall, SouthClassroom, {448313, -8003, 199475, 425784, -3404, 199220}, 0},
    {SouthClassroom, OtherworldEastHall, {579417, -10936, 231074, 569176, -4096, 223092}, 0}
};
static const CameraPose *doorView;
static const CameraPose otherworldArrival = {-72982, -14626, -228551, -77354, -3034, -252059};
static bool eastLabRoad, northPassageRoad;
enum { PassageRoadSplit = -243712 }; // Q4(-59.5 m), converted to Q12.
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

enum { ChaseArea, ChaseAlongX, ChaseAlongZ };

typedef struct {
    int16_t minX, maxX, minZ, maxZ;
    uint8_t axis;
} ChaseRoad;

// Horizontal bounds in the original camera roads' Q4 units.
static const ChaseRoad chaseRoads[MusicRoom] = {
    [FirstFloor]    = {1710, 1756,   680, 1169, ChaseAlongZ},
    [SouthHall]     = {2086, 2393,   294,  337, ChaseAlongX},
    [NorthHall]     = {1982, 2289,  3043, 3089, ChaseAlongX},
    [Courtyard]     = {-436, -196, -1096, -848, ChaseArea},
    [OtherworldCourtyard] = {-436, -196, -1096, -848, ChaseArea},
    [OtherworldLobby] = {1572, 1572, 2188, 2272, ChaseAlongZ},
    [OtherworldSideHall] = {2108, 2364, 2187, 2291, ChaseAlongX},
    [OtherworldEastHall] = {1710, 1756, 964, 1169, ChaseAlongZ},
    [NorthClassroom] = {2182, 2265, 1505, 1681, ChaseArea},
    [SouthClassroom] = {2212, 2297, 867, 1041, ChaseArea},
    [EastCorridor]  = {-814, -771,   684, 1168, ChaseAlongZ},
    [SchoolCorridor] = {-473, -166,   294,  345, ChaseAlongX},
    [UpperNorthHall] = {-545, -238,  3038, 3081, ChaseAlongX},
    [EquipmentRoom] = {-1581, -1566, 1560, 1641, ChaseAlongZ},
    [BasementStair]  = {859, 907, -380, -256, ChaseArea},
    [BoilerRoom]     = {347, 347, -328, -224, ChaseAlongZ}
};

static void advanceRoadCamera(void)
{
    SchoolRoom room = roomCurrent();
    if (room == Infirmary) {
        updateInfirmaryCamera();
        return;
    }
    if (room == ChemistryLab) {
        // MAP1_S01 switches between two stationary chase roads in the lab.
        int split = -246784;
        eastLabRoad = x > split + (eastLabRoad ? -1433 : 1433);
        return;
    }
    if (room == TowerPassage) {
        northPassageRoad = z > PassageRoadSplit + (northPassageRoad ? -1433 : 1433);
        // MAP1_S00 uses a west-side rail and a stationary road at the
        // ramp. Keep the moving eye on its side of the actor clearance.
        const int minZ = -258048, maxZ = -237984, railX = -248832;
        int along = 2*cosLerp((yaw+2048-136)*8);
        int ideal = limitToRoad(z+along, minZ, maxZ) - z;
        int offset = chaseAxisOffset(ideal, railX-x, roadEyeZ-z, cosLerp(yaw*8), sinLerp(yaw*8));
        int target = limitToRoad(z+offset, minZ, maxZ);
        roadEyeZ += limitToRoad((target-roadEyeZ)/7, -288, 288);
        return;
    }
    if (room == MusicRoom || room == ClockTower || room == FarClockTower || otherworldIntroActive()) return;
    // Leave the entry view as Harry walks out along the road. The small
    // chase road places the ideal eye two metres away, twelve degrees to
    // one side of the heading opposite Harry (vcMakeIdealCamPosUseVC_ROAD_DATA).
    int entryHold = room == BasementStair || room == BoilerRoom ? 3072 : 8192;
    if (abs(x-entryX) > entryHold || abs(z-entryZ) > entryHold ||
        (room == EquipmentRoom && x < EquipmentAisleSplit)) followingRoad = true;
    if (!followingRoad) return;
    int angle = (yaw + 2048 - 136)*8;
    int sx = x + 2*sinLerp(angle), sz = z + 2*cosLerp(angle);
    static const ChaseRoad basementRamp = {913, 913, -331, -284, ChaseAlongZ};
    static const ChaseRoad eastLowerHall = {1710, 1756, 762, 964, ChaseAlongZ};
    const ChaseRoad *road = &chaseRoads[room == OtherworldNorthHall ? NorthHall : room];
    if (room == BasementStair && x >= 57*4096) road = &basementRamp;
    if (room == OtherworldEastHall && z < eastLowerHall.maxZ*256) road = &eastLowerHall;
    int minX = road->minX*256, maxX = road->maxX*256;
    int minZ = road->minZ*256, maxZ = road->maxZ*256;
    sx = limitChaseAxis(sx, minX, maxX);
    sz = limitChaseAxis(sz, minZ, maxZ);
    if (road->axis != ChaseArea) {
        int facingX = sinLerp(yaw*8), facingZ = cosLerp(yaw*8);
        if (road->axis == ChaseAlongX) {
            int offset = chaseAxisOffset(sx-x, sz-z, roadEyeX-x, facingX, facingZ);
            sx = limitChaseAxis(x+offset, minX, maxX);
        } else {
            int offset = chaseAxisOffset(sz-z, sx-x, roadEyeZ-z, facingZ, facingX);
            sz = limitChaseAxis(z+offset, minZ, maxZ);
        }
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
    unsigned next = 0, mostPairs = 0;
    for (unsigned i = 0; i < batchCount; i++) {
        Batch *b = &batches[i];
        if (fread(b, sizeof *b, 1, file) != 1 || b->palette >= 15 ||
            (b->sides != 3 && b->sides != 4) || b->count % b->sides ||
            b->count > 1500-next || fread(corners+next, sizeof(Corner), b->count, file) != b->count)
            return false;
        unsigned count = b->count/2;
        if (mostPairs < count) mostPairs = count;
        next += b->count;
    }
    for (unsigned i = 0; i < next; i++) {
        if (corners[i].vertex >= vertexCount)
            return false;
    }
    VertexPair *buffer = mostPairs ? malloc(mostPairs*sizeof(*buffer)) : NULL;
    if (mostPairs && !buffer) return false;
    free(pairs);
    pairs = buffer;
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
        collisionMove(&x, &z, (sx*distance) >> 12, (sz*distance) >> 12, floor.height);
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

bool playerLoadCourtyardLook(FILE *file)
{
    return fread(frames+firstFrame[LookAround], sizeof frames[0],
                 frameCount[LookAround], file) == frameCount[LookAround];
}

bool playerLoadLadder(FILE *file, bool ascent)
{
    unsigned count = frameCount[DescendLadder] + (ascent ? frameCount[AscendLadder] : 0);
    if (fread(frames+firstFrame[DescendLadder], sizeof frames[0], count, file) != count) return false;
    ladderAscentReady = ascent;
    return true;
}

bool playerLadderActive(void)
{
    return ladderAge >= 0;
}

static bool onLadder(void)
{
    return ladderAge >= (ladderAscending ? LadderAscentMotion : 0);
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
    static const unsigned scripted[] = {
        [PlayerKneel] = Kneel, [PlayerRise] = Rise, [PlayerPour] = Pour,
        [PlayerLookAround] = LookAround, [PlayerScriptedWalk] = WalkForward
    };
    unsigned next = action == PlayerMove ?
        locomotion(roomVersion >= 3 ? turn : 0, roomVersion >= 4 ? walk : 0, dt) : scripted[action];
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
        frameQ12 += dt * frameRate[clip] - (clip == LookAround ? dt/10 : 0);
        if (clip < Kneel)
            frameQ12 %= frameCount[clip]*4096;
        else if (frameQ12 >= (frameCount[clip]-1)*4096)
            frameQ12 = (frameCount[clip]-1)*4096;
    }
    return true;
}

void playerScriptedPose(int worldX, int worldZ, int heading)
{
    x = worldX;
    z = worldZ;
    yaw = heading;
}

static void warp(int worldX, int worldZ, int heading)
{
    playerScriptedPose(worldX, worldZ, heading);
    speed = 0;
    clip = Idle;
    frameQ12 = blendQ12 = 0;
}

void playerFinishOtherworld(bool skipped)
{
    warp(skipped ? -74465 : -74458, skipped ? -254935 : -254955, 3584);
    doorView = &otherworldArrival;
    entryX = x;
    entryZ = z;
    roadEyeX = doorView->x;
    roadEyeZ = doorView->z;
    followingRoad = false;
    doorRail = true;
}

static void warpToLadder(void)
{
    // Both MAP1_S00 callbacks show the same shaft at point 23.
    warp(-230768, -225689, 0);
    clip = ladderAscending ? AscendLadder : DescendLadder;
    ladderHeight = ladderAscending ? -9322 : -9420;
}

bool playerLadderTick(bool skip, SchoolRoom *arrival)
{
    // The original ascent starts with a fade over the current passage view.
    int start = ladderAscending ? LadderAscentMotion : 5;
    int fadeOut = ladderAscending ? 259 : 216;
    int end = ladderAscending ? 310 : 264;
    if (++ladderAge >= end || (skip && ladderAge > start)) {
        ladderAge = -1;
        if (ladderAscending) {
            *arrival = ladderTower;
        } else {
            northPassageRoad = ladderTower == FarClockTower;
            roadEyeZ = northPassageRoad ? -252672 : -259584;
            warp(-245760, northPassageRoad ? -230604 : -260915, northPassageRoad ? 2048 : 0);
        }
        setBrightness(1, -16);
        return true;
    }
    if (ladderAscending && ladderAge < start) {
        setBrightness(1, -16 * limitToRoad(ladderAge, 0, LadderRevealVBlanks) / LadderRevealVBlanks);
        return false;
    }
    if (ladderAscending && ladderAge == start) warpToLadder();

    int motion = ladderAge - start;
    // The PS1 updates the climb and clip on alternating VBlanks.
    bool moved = motion >= 0 && !(motion & 1);
    if (moved) {
        unsigned steps = (unsigned)motion/2+1;
        int distance = steps*4021/100;
        ladderHeight = ladderAscending ? -9322-distance : -9420+distance;
        frameQ12 = steps*136*frameRate[clip] % (frameCount[clip]*4096);
    }
    int darkness = 0;
    if (ladderAge < start + LadderRevealVBlanks)
        darkness = 16 * (start + LadderRevealVBlanks - ladderAge) / LadderRevealVBlanks;
    else if (ladderAge > fadeOut)
        darkness = 16 * (ladderAge-fadeOut) / (end-fadeOut);
    setBrightness(1, -limitToRoad(darkness, 0, 16));
    return moved;
}

void playerWarpForMedal(void)
{
    warp(-100*4096, 82329, 3840);
}

void playerWarpForPour(void)
{
    warp(-242483, 405094, 0);
}

struct StairExit {
    SchoolRoom from, to;
    int triggerX, triggerZ, halfWidth, halfDepth;
    int arrivalX, arrivalZ, arrivalYaw;
    int eyeX, eyeZ;
};

// TouchAabb stair exits use their destination map point as the arrival
// pose. MAP1_S00 points 80 and 86 connect the basement flight.
static const StairExit stairExits[] = {
    {EastCorridor, FirstFloor, -184320, 174899, 6144, 6144, 454656, 188006, 3072, 441876, 179657},
    {FirstFloor, EastCorridor, 462848, 188006, 6144, 6144, -192512, 174899, 3072, -204178, 184306},
    {FirstFloor, BasementStair, 462848, 174489, 6144, 6144, 237568, -77824, 3072, 224047, -85118},
    {BasementStair, FirstFloor, 245760, -77824, 6144, 6144, 454656, 174489, 3072, 442003, 182974}
};

void playerEnterRoom(SchoolRoom from, RoomExit exit)
{
    doorView = NULL;
    SchoolRoom to = roomCurrent();
    if (to == OtherworldCourtyard && from == FarClockTower) {
        warp(-54556, -278859, 3584);
        return;
    }
    if (to == ClockTower || to == FarClockTower) {
        // MAP1_S00 points 19–21, with the inward offset from SysState_LoadRoom.
        bool southAscent = from == TowerPassage && to == ClockTower;
        warp(to == ClockTower ? -409600 : -573440, southAscent ? -247398 : -244121,
             southAscent ? 0 : 2048);
        return;
    }
    if (to == TowerPassage) {
        ladderTower = from;
        ladderAscending = false;
        ladderAge = 0;
        warpToLadder();
        return;
    }
    const Door *d = exit.arrival;
    for (unsigned j = 0; j < sizeof doorViews/sizeof doorViews[0]; j++) {
        const DoorView *view = &doorViews[j];
        if (view->room == to && view->from == from &&
            (!view->doorZ || (d && d->z == view->doorZ))) {
            doorView = &view->camera;
            break;
        }
    }
    const StairExit *stairs = exit.stairs;
    if (stairs) {
        warp(stairs->arrivalX, stairs->arrivalZ, stairs->arrivalYaw);
        entryX = x;
        entryZ = z;
        followingRoad = doorRail = false;
        roadEyeX = stairs->eyeX;
        roadEyeZ = stairs->eyeZ;
        return;
    }
    if (!d) return;
    int heading = d->nx ? (d->nx > 0 ? 1024 : 3072) : (d->nz > 0 ? 0 : 2048);
    warp(d->x + 1638*d->nx, d->z + 1638*d->nz, heading);
    entryX = x;
    entryZ = z;
    followingRoad = false;
    if (doorView) {
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
}

static bool facingEdge(int edgeX, int edgeZ, int halfWidth, int nx, int nz);

RoomExit playerDestination(bool action)
{
    SchoolRoom room = roomCurrent();
    for (unsigned i = 0; i < sizeof stairExits/sizeof stairExits[0]; i++) {
        const StairExit *exit = &stairExits[i];
        if (exit->from == room && abs(x - exit->triggerX) <= exit->halfWidth &&
            abs(z - exit->triggerZ) <= exit->halfDepth)
            return (RoomExit){.room = exit->to, .stairs = exit};
    }
    if (!action || roomVersion < 5) return (RoomExit){.room = room};
    // MAP1_S00 points 19 and 21 face opposite ways inside the towers.
    if ((room == ClockTower && facingEdge(-409600, -249036, 2048, 0, -1)) ||
        (room == FarClockTower && facingEdge(-573440, -242483, 2048, 0, 1)))
        return (RoomExit){.room = TowerPassage};
    if (room == FarClockTower && facingEdge(-573440, -249036, 1536, 0, -1))
        return (RoomExit){.room = OtherworldCourtyard};
    for (unsigned i = 0; i < schoolDoorCount; i++) {
        const Door *d = &schoolDoors[i];
        if (d->room == room && facingEdge(d->x, d->z, d->halfWidth, -d->nx, -d->nz))
            return (RoomExit){.room = d->other, .arrival = &schoolDoors[i ^ 1]};
    }
    return (RoomExit){.room = room};
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

bool playerStartAscent(void)
{
    if (roomCurrent() != TowerPassage || !ladderAscentReady) return false;
    // MAP1_S00 points 16 and 17 face the south and north passage ladders.
    if (facingEdge(-245760, -262144, 1536, 0, -1))
        ladderTower = ClockTower;
    else if (facingEdge(-245760, -229376, 1536, 0, 1))
        ladderTower = FarClockTower;
    else
        return false;
    ladderAscending = true;
    ladderAge = 0;
    return true;
}

bool playerFacingPiano(void)
{
    return roomVersion < 3 || facingEdge(-408371, 81510, 3072, 1, 0);
}

bool playerFacingLobbyFan(void)
{
    // MAP1_S02 point 46 lies at the end of the fan alcove.
    return roomCurrent() == OtherworldLobby && facingEdge(409600, 583270, 4864, 0, 1);
}

bool playerFacingTowerExit(void)
{
    // The interior side of MAP1_S00 point 20 uses the closed-door message.
    return roomCurrent() == ClockTower && facingEdge(-409600, -242483, 1536, 0, 1);
}

bool playerFacingBoiler(void)
{
    // MAP1_S00 point 30 faces west across a six-sixteenths-of-a-metre edge.
    return roomCurrent() == BoilerRoom && facingEdge(84377, -71680, 1536, -1, 0);
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

bool playerFacingCard(void)
{
    // MAP1_S02 points 29 and 30 approach the corner of the same desk.
    return facingEdge(569344, 406732, 1024, 1, 0) ||
           facingEdge(570572, 405504, 1024, 0, 1);
}

bool playerFacingScatteredCards(void)
{
    return facingEdge(569344, 407961, 2304, 1, 0) ||
           facingEdge(571801, 405504, 2304, 0, 1);
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
    bool classroom = room == NorthClassroom || room == SouthClassroom;
    bool corridor = classroom || room == SchoolCorridor || room == SouthHall || room == NorthHall ||
                    room == UpperNorthHall || room == OtherworldNorthHall || room == OtherworldLobby ||
                    room == OtherworldSideHall || room == OtherworldEastHall;
    bool equipment = room == EquipmentRoom;
    bool stairwell = room == EastCorridor || room == FirstFloor || room == BasementStair;
    bool boiler = room == BoilerRoom;
    bool infirmary = room == Infirmary;
    bool courtyard = room == Courtyard || room == OtherworldCourtyard;
    bool lab = room == ChemistryLab;
    int ground = collisionFloor(x, z).height;
    RoomOrigin roomOffset = roomOrigin();
    int origin = roomOffset.x, originZ = roomOffset.z;
    if (otherworldIntroActive()) {
        CameraPose view = otherworldIntroView();
        return setCameraPose(&view, origin, originZ);
    }
    if (room == ClockTower) {
        // Fixed road at (-99.9, -2.7, -60.3) in MAP1_S00.
        static const CameraPose view = {-409088, -11008, -246784, -411040, 5248, -246304};
        return setCameraPose(&view, origin, originZ);
    }
    if (room == FarClockTower) {
        // This MAP1_S00 road looks almost straight down. Use a horizontal
        // up vector along its bearing so GL can normalize the cross product.
        gluLookAtf32((-573184-origin)/2, 5504, (originZ+244736)/2,
                     (-573568-origin)/2, -2680, (originZ+244880)/2, -4096, 0, 1536);
        return 5504;
    }
    if (room == TowerPassage) {
        if (onLadder()) {
            static const CameraPose descent = {-231055, -23224, -226877, -231014, -7659, -221839};
            static const CameraPose ascent = {-232407, -7249, -225812, -227041, -22528, -223477};
            return setCameraPose(ladderAscending ? &ascent : &descent, origin, originZ);
        }
        if (northPassageRoad) {
            // The stationary north road looks downhill across both ramps.
            CameraPose view = {-245760, -6400 + ground/10, -252672,
                               x + sinLerp(yaw*8), ground-2304, z + cosLerp(yaw*8)};
            return setCameraPose(&view, origin, originZ);
        }
        int dx = x+248832, dz = z-roadEyeZ;
        int distance = sqrt32(dx*dx + dz*dz);
        int extra = 4*4096-distance;
        CameraPose view = {-248832, -1536, roadEyeZ,
                           x + sinLerp(yaw*8), ground-5632, z + cosLerp(yaw*8)};
        if (distance && extra > 0) {
            view.watchX += dx*extra/distance;
            view.watchZ += dz*extra/distance;
        }
        return setCameraPose(&view, origin, originZ);
    }
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
    } else if (corridor || stairwell || courtyard || equipment || boiler) {
        eyeX = roadEyeX;
        eyeZ = roadEyeZ;
        bool rail = doorRail && !followingRoad;
        // The authored southern hall road fixes the chase eye at Q4 -27.
        int chaseHeight = room == SouthHall ? 27*256 : 8600;
        bool highEye = room == OtherworldLobby || classroom;
        if (highEye) chaseHeight = 43*256;
        if (boiler) chaseHeight = (z < -68403 ? 27 : 38)*256;
        eyeHeight = ((rail ? 7987 : chaseHeight)-ground)/2;
        targetHeight = ((rail ? 3533 : highEye ? 4096 : 2816)-ground)/2;
        // The small road limits the camera to 2.5 metres above map origin.
        if (eyeHeight > 5120 && !highEye) eyeHeight = 5120;
    }
    int dx = x-eyeX, dz = z-eyeZ;
    int length = sqrt32(dx*dx + dz*dz);
    int extra = length < 4*4096 ? 4*4096-length : 0;
    int targetX = x + sinLerp(yaw*8), targetZ = z + cosLerp(yaw*8);
    if (length) {
        targetX += dx * extra / length;
        targetZ += dz * extra / length;
    }
    if ((corridor || stairwell || infirmary || courtyard || lab || equipment || boiler) && length && !(doorRail && !followingRoad)) {
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

typedef struct {
    uint32_t xy;
    int32_t z;
} PackedPosition;

ARM_CODE void playerDraw(unsigned firstPalette, unsigned fog, Flashlight *lamp)
{
    Matrix world[18];
    static PackedPosition positions[384];
    skeleton(world);
    for (unsigned i = 0; i < vertexCount; i++) {
        const Vertex *v = &vertices[i];
        const Matrix *m = &world[v->bone];
        // Pack once per transformed vertex, before the indexed corners reuse it.
        uint16_t px = (m->t[0] + dot(m->r[0], v->xyz)) * 8;
        uint16_t py = (m->t[1] + dot(m->r[1], v->xyz)) * -8;
        int16_t pz = (m->t[2] + dot(m->r[2], v->xyz)) * -8;
        positions[i].xy = px | ((uint32_t)py << 16);
        positions[i].z = pz;
    }

    glPushMatrix();
    // PS1 bone coordinates are in Q8. Convert root pose to room-local DS units.
    RoomOrigin origin = roomOrigin();
    int height = onLadder() ? ladderHeight : collisionFloor(x, z).height;
    if (lamp) {
        // Game_FlashlightAttribsSet attaches the light origin to the torso
        // and aims it from Harry's root heading, pitched down 15 degrees.
        static const int16_t offset[] = {-21, -72, 30};
        const Matrix *torso = &world[1];
        int bx = torso->t[0] + dot(torso->r[0], offset);
        int bz = torso->t[2] + dot(torso->r[2], offset);
        int c = cosLerp(yaw*8), s = sinLerp(yaw*8);
        int pitch = cosLerp(170*8);
        *lamp = (Flashlight){
            .x = (x >> 4)-(origin.x >> 4)+((c*bx+s*bz) >> 12),
            .y = (height >> 4)+torso->t[1]+dot(torso->r[1], offset),
            .z = (z >> 4)-(origin.z >> 4)+((c*bz-s*bx) >> 12),
            .dx = (s*pitch) >> 12, .dy = sinLerp(170*8), .dz = (c*pitch) >> 12
        };
    }
    glTranslatef32(((x >> 4)-(origin.x >> 4))*8, -height/2,
                   ((origin.z >> 4)-(z >> 4))*8);
    glRotateYi(-yaw*8);
    glPolyFmt(POLY_ALPHA(31) | POLY_CULL_FRONT | fog);
    const Corner *c = corners;
    for (unsigned i = 0; i < batchCount; i++) {
        const Batch *b = &batches[i];
        GFX_PAL_FORMAT = 2*(firstPalette + b->palette);
        glBegin(b->sides == 3 ? GL_TRIANGLES : GL_QUADS);
        unsigned count = b->count/2;
        for (unsigned j = 0; j < count; j++, c += 2) {
            VertexPair *pair = &pairs[j];
            pair->commands = FIFO_COMMAND_PACK(FIFO_TEX_COORD, FIFO_VERTEX16, FIFO_TEX_COORD, FIFO_VERTEX16);
            for (unsigned k = 0; k < 2; k++) {
                const PackedPosition *v = &positions[c[k].vertex];
                pair->parameters[k][0] = ((uint32_t)c[k].v << 20) | ((uint32_t)c[k].u << 4);
                pair->parameters[k][1] = v->xy;
                pair->parameters[k][2] = v->z;
            }
        }
        if (count) {
            unsigned bytes = count*sizeof(*pairs);
            DC_FlushRange(pairs, bytes);
            while (dmaBusy(0)) {}
            dmaSetParams(0, pairs, (void *)&GFX_FIFO, DMA_FIFO | (bytes/sizeof(uint32_t)));
            while (dmaBusy(0)) {}
        }
        if (b->count & 1) {
            const PackedPosition *v = &positions[c->vertex];
            glTexCoord2t16(c->u << 4, c->v << 4);
            GFX_VERTEX16 = v->xy;
            GFX_VERTEX16 = v->z;
            c++;
        }
        glEnd();
    }
    glPopMatrix(1);
}
