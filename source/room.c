#include <nds.h>
#include <string.h>
#include "room.h"
#include "player.h"

typedef struct {
    uint32_t offset;
    uint16_t width, firstPalette;
    uint32_t palettes;
} Page;

typedef struct {
    uint8_t state, page, palette, flags;
    uint32_t quads;
} Batch;

typedef struct {
    int16_t x, y, z;
    uint8_t u, v;
} Vertex;

static struct {
    char magic[4];
    uint16_t pages, batches;
    uint32_t textureBytes, paletteBytes;
} header;
static Page pages[4];
static Batch batches[128];
static Vertex *vertices;

static bool upload(FILE *file, void *vram, unsigned size)
{
    // FAT reads may use byte stores. Stage before writing to halfword-only VRAM.
    uint32_t buf[512];
    for (unsigned pos = 0; pos < size;) {
        unsigned n = size-pos < sizeof buf ? size-pos : sizeof buf;
        if (fread(buf, 1, n, file) != n)
            return false;
        memcpy((uint8_t *)vram + pos, buf, n);
        pos += n;
    }
    return true;
}

bool roomLoad(FILE *file)
{
    if (fread(&header, sizeof header, 1, file) != 1 ||
        !((!memcmp(header.magic, "SHR1", 4) && header.pages == 3) ||
          (!memcmp(header.magic, "SHR2", 4) && header.pages == 4)) ||
        header.batches > 128 || !header.batches ||
        header.textureBytes > 128*1024 || (header.textureBytes & 7) ||
        header.paletteBytes > 64*1024 || (header.paletteBytes & 31) ||
        fread(pages, sizeof(Page), header.pages, file) != header.pages)
        return false;
    for (unsigned i = 0; i < header.pages; i++) {
        const Page *p = &pages[i];
        if ((p->width != 128 && p->width != 256) || (p->offset & 7) ||
            p->offset > header.textureBytes || p->width*128u > header.textureBytes-p->offset ||
            p->firstPalette > header.paletteBytes/32 ||
            p->palettes > header.paletteBytes/32 - p->firstPalette)
            return false;
    }
    vramSetBankC(VRAM_C_LCD);
    vramSetBankE(VRAM_E_LCD);
    if (!upload(file, VRAM_C, header.textureBytes) || !upload(file, VRAM_E, header.paletteBytes))
        return false;
    vramSetBankC(VRAM_C_TEXTURE_SLOT0);
    vramSetBankE(VRAM_E_TEX_PALETTE);
    if (header.pages == 4 &&
        (pages[3].width != 256 || pages[3].palettes != 15 || !playerLoad(file)))
        return false;

    // One tightly packed stream in RAM. The file length bounds every batch read.
    long start = ftell(file);
    if (start < 0 || fseek(file, 0, SEEK_END)) return false;
    long bytes = ftell(file) - start - (long)(header.batches * sizeof(Batch));
    if (bytes <= 0 || bytes > 6144*(long)sizeof(Vertex) || bytes % (4*sizeof(Vertex)) ||
        fseek(file, start, SEEK_SET))
        return false;
    vertices = malloc(bytes);
    if (!vertices) return false;
    unsigned pos = 0, total = bytes / sizeof(Vertex);
    for (unsigned i = 0; i < header.batches; i++) {
        Batch *b = &batches[i];
        if (fread(b, sizeof *b, 1, file) != 1 || b->page >= header.pages ||
            b->state > 2 || b->flags > 3 || b->palette >= pages[b->page].palettes ||
            b->quads > (total-pos)/4 ||
            fread(vertices+pos, sizeof(Vertex)*4, b->quads, file) != b->quads)
            return false;
        pos += 4*b->quads;
    }
    return pos == total;
}

void roomDraw(const Medallion *medal)
{
    bool solved = medal->phase >= MedalLanded;
    bool gameplay = playerReady() && (medal->phase == MedalHanging || medal->phase == MedalFinished);
    // MAP1_S01 medallion cameras. Local origin (-100, 0, 20), 2 metres/unit.
    // Screen_CutsceneCameraStateUpdate masks the 320x240 picture at y=±96.
    // Shrink the frustum with the viewport to keep the original pixel scale.
    int border = gameplay ? 6 : 19;
    glViewport(0, border, 255, 191-border);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const int distance = gameplay ? 224 : medal->phase == MedalHanging ? 353 : medal->projection;
    const int near = 1024; // 0.25 scene units, Q12.
    int horizontal = 160 * near / distance;
    int vertical = (gameplay ? 112 : 96) * near / distance;
    glFrustumf32(-horizontal, horizontal, -vertical, vertical, near, inttof32(16));
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    if (gameplay) {
        // Room 39 at the piano, before and after the event warps Harry.
        if (solved) gluLookAt(-1, 1.5, 0.875, 0.2799, -0.25, -0.9482, 0, 1, 0);
        else gluLookAt(-1, 1.5, 0.875, 1.0226, -0.25, -0.4296, 0, 1, 0);
    } else if (solved)
        gluLookAt(-0.695, 1.30495, -1.065, 0.8, 0.07, -1.55, 0, 1, 0);
    else
        gluLookAt(1.35, 1.18495, -0.985, -0.185, 1.1, -2.265, 0, 1, 0);

    // vbSetWorldScreenMatrix applies the PS1 224/240 vertical correction
    // to camera-relative world coordinates before rotating them.
    int eyeY = gameplay ? floattof32(1.5) : solved ? floattof32(1.30495) : floattof32(1.18495);
    glTranslatef32(0, eyeY, 0);
    glScalef32(inttof32(1), 4096*224/240, inttof32(1));
    glTranslatef32(0, -eyeY, 0);

    glEnable(GL_TEXTURE_2D | GL_BLEND);
    glClearColor(0, 0, 0, 31);
    glClearPolyID(63);
    glClearDepth(0x7fff);
    glColor(RGB15(31,31,31));
    const Vertex *v = vertices;
    if (playerReady()) {
        const Page *skin = &pages[3];
        GFX_TEX_FORMAT = (skin->offset >> 3) | (TEXTURE_SIZE_256 << 20)
                         | (TEXTURE_SIZE_256 << 23) | (GL_RGB16 << 26);
        playerDraw(skin->firstPalette, medal->phase != MedalHanging);
    }
    for (unsigned i = 0; i < header.batches; i++) {
        const Batch *b = &batches[i];
        unsigned count = 4*b->quads;
        if (b->state == 0 || b->state == (solved ? 2 : 1)) {
            bool moving = b->state == 1 && medal->phase >= MedalSwing;
            if (moving) {
                // SHR1 stores the hanging model with its Q8 pivot baked in.
                glPushMatrix();
                glTranslatef32(2048, -medal->y / 2, (20 * 4096 - medal->z) / 2);
                glRotateXi(medal->angle * 8);
                glTranslatef32(-2048, -5488, 7640);
            }
            const Page *p = &pages[b->page];
            unsigned width = p->width == 128 ? TEXTURE_SIZE_128 : TEXTURE_SIZE_256;
            GFX_TEX_FORMAT = (p->offset >> 3) | (width << 20) | (TEXTURE_SIZE_256 << 23)
                             | (GL_RGB16 << 26) | ((b->flags & 2) ? GL_TEXTURE_COLOR0_TRANSPARENT : 0);
            GFX_PAL_FORMAT = (p->firstPalette + b->palette) * 2;
            // PS1 visible faces wind clockwise in these screen coordinates.
            glPolyFmt(POLY_ALPHA((b->flags & 1) ? 16 : 31) | POLY_CULL_FRONT);
            glBegin(GL_QUADS);
            for (unsigned j = 0; j < count; j++) {
                glTexCoord2t16(v[j].u << 4, v[j].v << 4);
                glVertex3v16(v[j].x, v[j].y, v[j].z);
            }
            glEnd();
            if (moving) glPopMatrix(1);
        }
        v += count;
    }
    glFlush(0);
}
