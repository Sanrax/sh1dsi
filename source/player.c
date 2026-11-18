// SPDX-License-Identifier: GPL-3.0-only
// Animation interpolation adapted from Anim_BonesUpdate.
// Copyright (C) 2026 shdecompilations
#include <nds.h>
#include "player.h"

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
static int8_t idle[40][156];
static Vertex vertices[384];
static Corner corners[1500];
static Batch batches[30];
static uint16_t vertexCount, batchCount;
static unsigned animUs;
static bool ready, halfFrame;

bool playerReady(void)
{
    return ready;
}

bool playerLoad(FILE *file)
{
    if (fread(&vertexCount, 2, 1, file) != 1 || vertexCount > 384 || !vertexCount ||
        fread(&batchCount, 2, 1, file) != 1 || batchCount > 30 || !batchCount ||
        fread(bones, sizeof bones, 1, file) != 1 || fread(idle, sizeof idle, 1, file) != 1 ||
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

bool playerTick(void)
{
    if (!ready) return false;
    halfFrame = !halfFrame;
    if (halfFrame) return false;
    // HarryAnim_Idle loops over keyframes 503..542 at ten keyframes/second.
    animUs = (animUs + 33366) % 4000000;
    return true;
}

static int dot(const int16_t a[3], const int16_t b[3])
{
    return (a[0]*b[0] + a[1]*b[1] + a[2]*b[2]) >> 12;
}

static void skeleton(Matrix world[18])
{
    unsigned frame = animUs / 100000;
    int alpha = (animUs % 100000) * 4096 / 100000;
    const int8_t *a = idle[frame], *b = idle[(frame+1) % 40];
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

void playerDraw(unsigned firstPalette, bool afterPuzzle)
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
    glTranslatef32(afterPuzzle ? 0 : -8, 0, afterPuzzle ? -200 : 80);
    glRotateYi(afterPuzzle ? 2048 : -8192);
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
