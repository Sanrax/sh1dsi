// SPDX-License-Identifier: GPL-3.0-only
// Animation interpolation adapted from Anim_BonesUpdate.
// Copyright (C) 2026 shdecompilations
#include <nds.h>
#include "player.h"
#include "collision.h"
#include "room.h"

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
enum { Idle, TurnLeft, TurnRight, WalkForward, WalkBackward, Kneel, Rise };
static const unsigned firstFrame[] = {0, 40, 53, 66, 89, 113, 128};
static const unsigned frameCount[] = {40, 13, 13, 23, 24, 15, 12};
static const unsigned frameRate[] = {10, 15, 15, 22, 23, 15, 15};
static const unsigned blendRate[] = {5, 50, 50, 50, 20, 10, 10};
static int8_t frames[140][156];
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

static int limitToRoad(int value, int low, int high)
{
    return value < low ? low : value > high ? high : value;
}

static void advanceRoadCamera(void)
{
    if (roomCurrent() == MusicRoom) return;
    // Leave the entry view as Harry walks out along the road. The small
    // chase road places the ideal eye two metres away, twelve degrees to
    // one side of the heading opposite Harry (vcMakeIdealCamPosUseVC_ROAD_DATA).
    if (abs(x-entryX) > 8192 || abs(z-entryZ) > 8192) followingRoad = true;
    if (!followingRoad) return;
    int angle = (yaw + 2048 - 136)*8;
    int sx = x + 2*sinLerp(angle), sz = z + 2*cosLerp(angle);
    if (roomCurrent() == SchoolCorridor) {
        sx = limitToRoad(sx, -121088, -42496);
        sz = limitToRoad(sz, 75264, 88320);
    } else {
        sx = limitToRoad(sx, -208384, -197376);
        sz = limitToRoad(sz, 175104, 299008);
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
        action == PlayerKneel ? Kneel : Rise;
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

// MAP_POINTS 41/39 and 36/51. Each normal points from the threshold into
// its room. Trigger half-widths are in Q12 metres.
typedef struct {
    SchoolRoom room, other;
    int x, z, nx, nz, halfWidth;
} Door;
static const Door doors[] = {
    {MusicRoom, SchoolCorridor, -417792, 97075, 0, -1, 2304},
    {SchoolCorridor, MusicRoom, -116326, 75366, 0, 1, 2304},
    {SchoolCorridor, EastCorridor, -42598, 81920, -1, 0, 3584},
    {EastCorridor, SchoolCorridor, -210534, 201113, 1, 0, 3584}
};

void playerEnterRoom(SchoolRoom from)
{
    for (unsigned i = 0; i < sizeof doors / sizeof doors[0]; i++) {
        const Door *d = &doors[i];
        if (d->room == roomCurrent() && d->other == from) {
            int heading = d->nx ? (d->nx > 0 ? 1024 : 3072) : (d->nz > 0 ? 0 : 2048);
            warp(d->x + 1638*d->nx, d->z + 1638*d->nz, heading);
            entryX = x;
            entryZ = z;
            followingRoad = false;
            doorRail = from == EastCorridor || d->room == EastCorridor;
            // Camera entry anchors for MAP_POINTS 39, 51 and 36.
            roadEyeX = !doorRail ? -116992 : d->room == EastCorridor ? -209108 : -41644;
            roadEyeZ = !doorRail ? 83807 : z;
            return;
        }
    }
}

SchoolRoom playerDoorDestination(void)
{
    SchoolRoom room = roomCurrent();
    if (roomVersion < 5) return room;
    int rayX = sinLerp(yaw*8)/2, rayZ = cosLerp(yaw*8)/2;
    for (unsigned i = 0; i < sizeof doors / sizeof doors[0]; i++) {
        const Door *d = &doors[i];
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

bool playerFacingPiano(void)
{
    if (roomVersion < 3) return true;
    // Event_CollideObbFacingCheck, MAP_POINTS[16]. Intersect Harry's
    // half-metre facing ray with the piano's north-south interaction edge.
    int dx = -408371 - x, dz = 81510 - z;
    int rayX = sinLerp(yaw*8)/2, rayZ = cosLerp(yaw*8)/2;
    if (rayX <= 0 || dx < 0 || dx > rayX)
        return false;
    int along = rayZ*dx - dz*rayX;
    return abs(along) <= 3072 * rayX;
}

bool playerFacingMedallion(void)
{
    // MAP_POINTS[17] and Event_CollideFacingCheck. The origin of the
    // 0.8-metre sector is one eighth of a metre behind Harry.
    if (roomVersion < 5) return false;
    int sx = sinLerp(yaw*8), sz = cosLerp(yaw*8);
    int dx = -404684 - x + sx/8, dz = 93388 - z + sz/8;
    if (abs(dx) > 3276 || abs(dz) > 3276 || dx*dx+dz*dz > 3276*3276)
        return false;
    int along = dx*sx + dz*sz;
    // tan(30 degrees) in Q12. The cross product is the lateral component.
    return along > 0 && abs(dx*sz-dz*sx) <= (int)(((int64_t)along*2365) >> 12);
}

int playerSetCamera(void)
{
    // vcMakeNormalWatchTgtPos / vcSetWatchTgtXzPos, small chase road.
    // Its one-metre watch circle extends away from the room camera whenever
    // the distance to Harry is below four metres.
    bool corridor = roomCurrent() == SchoolCorridor;
    bool east = roomCurrent() == EastCorridor;
    int ground = collisionFloor(x, z).height;
    int origin = roomOriginX(), originZ = roomOriginZ();
    if (roomCurrent() == MusicRoom && x >= -419840 && x <= -416768 && z >= 94976 && z <= 98304) {
        // Fixed-angle road looking back through the music-room doorway.
        gluLookAtf32((-420864-origin)/2, 768, (81920-82688)/2,
                     (-414400-origin)/2, 3904, (81920-96368)/2, 0,4096,0);
        return 768;
    }
    int eyeX = -417792, eyeZ = 74752, eyeHeight = 6144, targetHeight = -1024;
    if (corridor || east) {
        eyeX = roadEyeX;
        eyeZ = roadEyeZ;
        bool rail = doorRail && !followingRoad;
        eyeHeight = ((rail ? 7987 : 8600)-ground)/2;
        targetHeight = ((rail ? 3533 : 2816)-ground)/2;
    }
    int dx = x-eyeX, dz = z-eyeZ;
    int length = sqrt32(dx*dx + dz*dz);
    int extra = length < 4*4096 ? 4*4096-length : 0;
    int targetX = x + sinLerp(yaw*8), targetZ = z + cosLerp(yaw*8);
    if (length) {
        targetX += dx * extra / length;
        targetZ += dz * extra / length;
    }
    if ((corridor || east) && length && !(doorRail && !followingRoad)) {
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
    glTranslatef32(((x-roomOriginX()) >> 4)*8, -collisionFloor(x, z).height/2, ((roomOriginZ() >> 4)-(z >> 4))*8);
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
