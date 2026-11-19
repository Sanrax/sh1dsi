#include <nds.h>
#include <string.h>
#include "room.h"
#include "player.h"
#include "collision.h"
#include "pickup.h"

typedef struct {
    uint32_t offset;
    uint16_t width, firstPalette;
    uint32_t palettes;
} Page;

typedef struct {
    uint8_t state, page, palette, flags;
    uint32_t quads;
} Batch;

enum { StaticBatch, HangingSilverBatch, FallenSilverBatch, DrinkBatch, KitBatch };

typedef struct {
    int16_t x, y, z;
    uint8_t u, v;
} Vertex;

static struct {
    char magic[4];
    uint16_t pages, batches;
    uint32_t textureBytes, paletteBytes;
} header;
static Page pages[4], skin;
static SchoolRoom current = MusicRoom;

// MAP1_S00 and MAP1_S01 door points. Normals point into the room.
const Door schoolDoors[] = {
    {MusicRoom, SchoolCorridor, -417792, 97075, 0, -1, 2304},
    {SchoolCorridor, MusicRoom, -116326, 75366, 0, 1, 2304},
    {SchoolCorridor, EastCorridor, -42598, 81920, -1, 0, 3584},
    {EastCorridor, SchoolCorridor, -210534, 201113, 1, 0, 3584},
    {FirstFloor, SouthHall, 436633, 201113, 1, 0, 3584},
    {SouthHall, FirstFloor, 612761, 81920, -1, 0, 3584},
    {SouthHall, Infirmary, 591462, 75366, 0, 1, 2304},
    {Infirmary, SouthHall, 233472, 589004, 0, -1, 2304},
    {SouthHall, Courtyard, 573440, 95027, 0, -1, 4864},
    {Courtyard, SouthHall, -80691, -282214, 0, 1, 4864}
};
const unsigned schoolDoorCount = sizeof schoolDoors/sizeof schoolDoors[0];

SchoolRoom roomCurrent(void) { return current; }
int roomOriginX(SchoolRoom room)
{
    switch (room) {
    case MusicRoom: return -100*4096;
    case Courtyard:
    case SchoolCorridor: return -20*4096;
    case EastCorridor: return -60*4096;
    case FirstFloor: return 100*4096;
    case Infirmary: return 60*4096;
    case SouthHall: return 140*4096;
    }
    return 0;
}

int roomOriginZ(SchoolRoom room)
{
    if (room == Infirmary) return 140*4096;
    if (room == Courtyard) return -60*4096;
    return (room == EastCorridor || room == FirstFloor ? 60 : 20)*4096;
}

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

bool roomLoad(FILE *file, SchoolRoom room)
{
    bool corridor = room != MusicRoom;
    const char *magic = "SHC1";
    if (room == FirstFloor) magic = "SHF1";
    if (room == EastCorridor) magic = "SHE1";
    if (room == SouthHall) magic = "SHS1";
    if (room == Infirmary) magic = "SHI1";
    if (room == Courtyard) magic = "SHY1";
    if (fread(&header, sizeof header, 1, file) != 1) return false;
    bool firstFloorObstacles = room == FirstFloor && !memcmp(header.magic, "SHF3", 4);
    bool southHallObstacles = room == SouthHall && !memcmp(header.magic, "SHS2", 4);
    bool towerFloors = room == Courtyard && !memcmp(header.magic, "SHY2", 4);
    bool latchText = firstFloorObstacles || (room == FirstFloor && !memcmp(header.magic, "SHF2", 4));
    bool medical = room == Infirmary && !memcmp(header.magic, "SHI3", 4);
    bool items = medical || (room == Infirmary && !memcmp(header.magic, "SHI2", 4));
    enum { ItemPageBytes = 64*256/2, ItemPageOffset = 128*1024-ItemPageBytes };
    if (items && header.textureBytes < ItemPageBytes) return false;
    unsigned sceneBytes = header.textureBytes - (items ? ItemPageBytes : 0);
    unsigned scenePages = room == SchoolCorridor ? 2 : 3;
    if (latchText) magic = firstFloorObstacles ? "SHF3" : "SHF2";
    if (southHallObstacles) magic = "SHS2";
    if (towerFloors) magic = "SHY2";
    if (items) magic = medical ? "SHI3" : "SHI2";
    if ((corridor ? (memcmp(header.magic, magic, 4) || header.pages != scenePages + items || !playerReady() ||
                     sceneBytes > skin.offset || header.paletteBytes > skin.firstPalette*32u) :
         !((!memcmp(header.magic, "SHR1", 4) && header.pages == 3) ||
           (!memcmp(header.magic, "SHR", 3) && header.magic[3] >= '2' && header.magic[3] <= '5' && header.pages == 4))) ||
        header.batches > 128 || !header.batches ||
        header.textureBytes > 128*1024 || (header.textureBytes & 7) ||
        header.paletteBytes > 64*1024 || (header.paletteBytes & 31) ||
        fread(pages, sizeof(Page), header.pages, file) != header.pages)
        return false;
    for (unsigned i = 0; i < header.pages; i++) {
        const Page *p = &pages[i];
        bool atlas = items && i == scenePages;
        if (atlas && (p->width != 64 || p->offset != sceneBytes || p->palettes != 4)) return false;
        unsigned limit = atlas ? header.textureBytes : sceneBytes;
        if ((!atlas && p->width != 128 && p->width != 256) || (p->offset & 7) ||
            p->offset > limit || p->width*128u > limit-p->offset ||
            p->firstPalette > header.paletteBytes/32 ||
            p->palettes > header.paletteBytes/32 - p->firstPalette)
            return false;
    }
    vramSetBankC(VRAM_C_LCD);
    vramSetBankE(VRAM_E_LCD);
    if (!upload(file, VRAM_C, sceneBytes)) return false;
    if (items) {
        // Keep the actor and pickup resident while loading the common items
        // into the last 8 KiB of texture bank C.
        if (skin.offset + 256*256/2 + 2048 > ItemPageOffset ||
            !upload(file, (uint8_t *)VRAM_C + ItemPageOffset, ItemPageBytes)) return false;
        pages[scenePages].offset = ItemPageOffset;
    }
    if (!upload(file, VRAM_E, header.paletteBytes)) return false;
    if (room == MusicRoom && header.pages == 4) {
        // Keep actor CLUTs above the largest streamed scenery palette.
        // The music room file also has item data directly after its skin.
        skin = pages[3];
        if (skin.palettes != 15 || skin.firstPalette + skin.palettes > 64 ||
            skin.firstPalette + skin.palettes > header.paletteBytes/32)
            return false;
        memcpy((uint8_t *)VRAM_E + 64*32, (uint8_t *)VRAM_E + skin.firstPalette*32, skin.palettes*32);
        skin.firstPalette = 64;
    }
    vramSetBankC(VRAM_C_TEXTURE_SLOT0);
    vramSetBankE(VRAM_E_TEX_PALETTE);
    if ((corridor || header.magic[3] >= '4') &&
        !collisionLoad(file, room, firstFloorObstacles || southHallObstacles || towerFloors)) return false;
    if (latchText && !latchLoad(file)) return false;
    if (room == MusicRoom && header.pages == 4 &&
        (pages[3].width != 256 || pages[3].palettes != 15 || !playerLoad(file, header.magic[3]-'0')))
        return false;

    if (header.magic[3] == '5' &&
        (header.textureBytes < 2048 || header.paletteBytes < 32 ||
         !pickupLoad(file, SilverMedallion, header.textureBytes-2048, header.paletteBytes/32-1)))
        return false;
    if (medical && (!pickupLoad(file, HealthDrink, ItemPageOffset, pages[scenePages].firstPalette+1) ||
                    !pickupLoad(file, FirstAidKit, ItemPageOffset, pages[scenePages].firstPalette)))
        return false;

    // One tightly packed stream in RAM. The file length bounds every batch read.
    long start = ftell(file);
    if (start < 0 || fseek(file, 0, SEEK_END)) return false;
    long bytes = ftell(file) - start - (long)(header.batches * sizeof(Batch));
    unsigned maxVerts = room == Courtyard ? 11000 : room == MusicRoom || room == SchoolCorridor ? 6144 : 8192;
    if (bytes <= 0 || bytes > maxVerts*(long)sizeof(Vertex) || bytes % (4*sizeof(Vertex)) ||
        fseek(file, start, SEEK_SET))
        return false;
    free(vertices);
    vertices = malloc(bytes);
    if (!vertices) return false;
    unsigned pos = 0, total = bytes / sizeof(Vertex);
    unsigned maxState = medical ? KitBatch : corridor ? StaticBatch : FallenSilverBatch;
    for (unsigned i = 0; i < header.batches; i++) {
        Batch *b = &batches[i];
        if (fread(b, sizeof *b, 1, file) != 1 || b->page >= header.pages ||
            b->state > maxState || b->flags > (room == Courtyard ? 7 : 3) || b->palette >= pages[b->page].palettes ||
            b->quads > (total-pos)/4 ||
            fread(vertices+pos, sizeof(Vertex)*4, b->quads, file) != b->quads)
            return false;
        pos += 4*b->quads;
    }
    current = room;
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
    int eyeY = solved ? floattof32(1.30495) : floattof32(1.18495);
    if (gameplay) {
        eyeY = playerSetCamera();
    } else if (solved)
        gluLookAt(-0.695, 1.30495, -1.065, 0.8, 0.07, -1.55, 0, 1, 0);
    else
        gluLookAt(1.35, 1.18495, -0.985, -0.185, 1.1, -2.265, 0, 1, 0);

    // vbSetWorldScreenMatrix applies the PS1 224/240 vertical correction
    // to camera-relative world coordinates before rotating them.
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
        GFX_TEX_FORMAT = (skin.offset >> 3) | (TEXTURE_SIZE_256 << 20)
                         | (TEXTURE_SIZE_256 << 23) | (GL_RGB16 << 26);
        playerDraw(skin.firstPalette);
    }
    if (current == EastCorridor || current == FirstFloor) glScalef32(8192, 8192, 8192);
    bool visible[] = {
        [StaticBatch] = true,
        [HangingSilverBatch] = !solved,
        [FallenSilverBatch] = solved && !pickupHidden(SilverMedallion),
        [DrinkBatch] = !pickupHidden(HealthDrink),
        [KitBatch] = !pickupHidden(FirstAidKit),
    };
    for (unsigned i = 0; i < header.batches; i++) {
        const Batch *b = &batches[i];
        unsigned count = 4*b->quads;
        if (visible[b->state]) {
            bool moving = b->state == HangingSilverBatch && medal->phase >= MedalSwing;
            if (moving) {
                // SHR1 stores the hanging model with its Q8 pivot baked in.
                glPushMatrix();
                glTranslatef32(2048, -medal->y / 2, (20 * 4096 - medal->z) / 2);
                glRotateXi(medal->angle * 8);
                glTranslatef32(-2048, -5488, 7640);
            }
            const Page *p = &pages[b->page];
            unsigned width = p->width == 64 ? TEXTURE_SIZE_64 :
                             p->width == 128 ? TEXTURE_SIZE_128 : TEXTURE_SIZE_256;
            GFX_TEX_FORMAT = (b->flags & 4) ? 0 :
                             (p->offset >> 3) | (width << 20) | (TEXTURE_SIZE_256 << 23)
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
    pickupDrawItem();
    glFlush(0);
}
