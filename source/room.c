#include <nds.h>
#include <string.h>
#include <sys/stat.h>
#include "room.h"
#include "player.h"
#include "collision.h"
#include "pickup.h"
#include "statue.h"

typedef struct {
    uint32_t offset;
    uint16_t width, firstPalette;
    uint32_t palettes;
} Page;

typedef struct {
    uint8_t state, page, palette, flags;
    uint32_t quads;
} Batch;

enum { StaticBatch, HangingSilverBatch, FallenSilverBatch, DrinkBatch, KitBatch, ChemicalBatch,
       IntactHandBatch, OpenHandBatch, GoldAtHandBatch, BoilerUnwiredBatch, BoilerReadyBatch, BoilerGlowBatch };

typedef struct {
    int16_t x, y, z;
    uint8_t u, v;
} Vertex;

_Static_assert(sizeof(Vertex) == 8, "room vertex layout");

static struct {
    char magic[4];
    uint16_t pages, batches;
    uint32_t textureBytes, paletteBytes;
} header;
static Page pages[4], skin;
enum { RoomEdgeHeights = 1, RoomFloorPlanes = 2, RoomWideVertices = 4, RoomGroundPlanes = 8, RoomBoilerEvent = 16, RoomDoorNotice = 32, RoomLadderAscent = 64 };
static SchoolRoom current = MusicRoom;
static RoomOrigin currentOrigin = {-100*4096, 20*4096};
static bool handPresent, handPuzzle, wideGeometry;

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
    {EastCorridor, UpperNorthHall, -210534, 292864, 1, 0, 3584},
    {UpperNorthHall, EastCorridor, -61030, 784384, -1, 0, 3584},
    {UpperNorthHall, ChemistryLab, -118374, 790937, 0, -1, 2304},
    {ChemistryLab, UpperNorthHall, -261324, 396492, 0, 1, 2304},
    {UpperNorthHall, EquipmentRoom, -131481, 790937, 0, -1, 2304},
    {EquipmentRoom, UpperNorthHall, -403865, 396492, 0, 1, 2304},
    {SouthHall, Courtyard, 573440, 95027, 0, -1, 4864},
    {Courtyard, SouthHall, -80691, -282214, 0, 1, 4864},
    {Courtyard, NorthHall, -80691, -215040, 0, -1, 4864},
    {NorthHall, Courtyard, 546816, 771276, 0, 1, 4864},
    {BasementStair, BoilerRoom, 219545, -70451, 1, 0, 2304},
    {BoilerRoom, BasementStair, 93388, -79872, -1, 0, 2304}
};
const unsigned schoolDoorCount = sizeof schoolDoors/sizeof schoolDoors[0];

SchoolRoom roomCurrent(void) { return current; }
bool roomHasHand(void) { return current == ChemistryLab && handPresent; }
bool roomCanDissolveHand(void) { return current == ChemistryLab && handPuzzle; }
RoomOrigin roomOrigin(void) { return currentOrigin; }

static RoomOrigin legacyRoomOrigin(SchoolRoom room)
{
    switch (room) {
    case MusicRoom: return (RoomOrigin){-100*4096, 20*4096};
    case EquipmentRoom: return (RoomOrigin){-100*4096, 100*4096};
    case Courtyard: return (RoomOrigin){-20*4096, -60*4096};
    case UpperNorthHall: return (RoomOrigin){-20*4096, 180*4096};
    case SchoolCorridor: return (RoomOrigin){-20*4096, 20*4096};
    case ChemistryLab: return (RoomOrigin){-60*4096, 100*4096};
    case EastCorridor: return (RoomOrigin){-60*4096, 60*4096};
    case FirstFloor: return (RoomOrigin){100*4096, 60*4096};
    case Infirmary: return (RoomOrigin){60*4096, 140*4096};
    case NorthHall: return (RoomOrigin){140*4096, 180*4096};
    case BasementStair:
    case BoilerRoom:
    case ClockTower:
    case FarClockTower:
    case TowerPassage: break; // These rooms require a stream origin.
    case SouthHall: return (RoomOrigin){140*4096, 20*4096};
    }
    return (RoomOrigin){0, 20*4096};
}

enum { MaxBatches = 128, MaxRoomVertices = 11000, ChunkQuads = 32 };
enum { MaxChunks = MaxRoomVertices/(4*ChunkQuads) + MaxBatches };
typedef struct {
    int16_t bounds[6]; // Minimum XYZ, then maximum XYZ.
    uint16_t words;
} GeometryChunk;

typedef struct {
    int64_t offset;
    int32_t normal[3];
    uint8_t corner[3];
} ClipPlane;

static Batch batches[MaxBatches];
static GeometryChunk chunks[MaxChunks];
static bool visibleChunks[MaxChunks], visibilityValid;
static int visibilityMatrix[16];
static unsigned batchWords[MaxBatches];
static uint32_t *geometry;

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

static unsigned packPosition(uint32_t **out, const Vertex *v, const Vertex *previous)
{
    uint32_t x = (uint16_t)v->x, y = (uint16_t)v->y, z = (uint16_t)v->z;
    unsigned command;
    uint32_t value;
    if (previous && v->z == previous->z) {
        command = FIFO_VERTEX_XY;
        value = x | (y << 16);
    } else if (previous && v->y == previous->y) {
        command = FIFO_VERTEX_XZ;
        value = x | (z << 16);
    } else if (previous && v->x == previous->x) {
        command = FIFO_VERTEX_YZ;
        value = y | (z << 16);
    } else {
        command = FIFO_VERTEX16;
        *(*out)++ = x | (y << 16);
        value = (uint32_t)(int32_t)v->z;
    }
    *(*out)++ = value;
    return command;
}

static uint32_t *loadGeometry(FILE *file, uint32_t *out, unsigned quads, GeometryChunk **chunk)
{
    Vertex vertices[4*ChunkQuads];
    while (quads) {
        unsigned count = quads < ChunkQuads ? quads : ChunkQuads;
        if (fread(vertices, sizeof(Vertex)*4, count, file) != count) return NULL;
        GeometryChunk *part = (*chunk)++;
        part->bounds[0] = part->bounds[3] = vertices[0].x;
        part->bounds[1] = part->bounds[4] = vertices[0].y;
        part->bounds[2] = part->bounds[5] = vertices[0].z;

        const uint32_t *start = out;
        // Restart positions at chunk boundaries so either span can be skipped.
        const Vertex *previous = NULL;
        for (unsigned i = 0; i < count*4; i += 2) {
            uint32_t *commands = out++;
            const Vertex *first = &vertices[i], *second = first+1;
            *out++ = ((uint32_t)first->v << 20) | ((uint32_t)first->u << 4);
            unsigned firstPosition = packPosition(&out, first, previous);
            *out++ = ((uint32_t)second->v << 20) | ((uint32_t)second->u << 4);
            unsigned secondPosition = packPosition(&out, second, first);
            *commands = FIFO_COMMAND_PACK(FIFO_TEX_COORD, firstPosition,
                                          FIFO_TEX_COORD, secondPosition);
            previous = second;
        }
        part->words = out-start;
        for (unsigned i = 1; i < count*4; i++) {
            const Vertex *v = &vertices[i];
            if (v->x < part->bounds[0]) part->bounds[0] = v->x;
            if (v->x > part->bounds[3]) part->bounds[3] = v->x;
            if (v->y < part->bounds[1]) part->bounds[1] = v->y;
            if (v->y > part->bounds[4]) part->bounds[4] = v->y;
            if (v->z < part->bounds[2]) part->bounds[2] = v->z;
            if (v->z > part->bounds[5]) part->bounds[5] = v->z;
        }
        quads -= count;
    }
    return out;
}

static long roomFileSize(FILE *file, long position)
{
    // An end seek discards read-ahead and can restart FAT reads mid-sector.
    struct stat info;
    int fd = fileno(file);
    if (fd >= 0 && fstat(fd, &info) == 0 && S_ISREG(info.st_mode))
        return info.st_size;
    if (fseek(file, 0, SEEK_END)) return -1;
    long size = ftell(file);
    return fseek(file, position, SEEK_SET) ? -1 : size;
}

static bool loadRoom(FILE *file, SchoolRoom room, bool scenery)
{
    bool corridor = room != MusicRoom;
    const char *magic = "SHC1";
    if (room == FirstFloor) magic = "SHF1";
    if (room == EastCorridor) magic = "SHE1";
    if (room == SouthHall) magic = "SHS1";
    if (room == NorthHall) magic = "SHU1";
    if (room == UpperNorthHall) magic = "SHH1";
    if (room == ChemistryLab) magic = "SHL1";
    if (room == EquipmentRoom) magic = "SHQ1";
    if (room == Infirmary) magic = "SHI1";
    if (fread(&header, sizeof header, 1, file) != 1) return false;
    bool hasOrigin = !memcmp(header.magic, "SHM2", 4);
    bool streamed = hasOrigin || !memcmp(header.magic, "SHM1", 4);
    if (!hasOrigin && (room == BasementStair || room == BoilerRoom ||
                       room == ClockTower || room == FarClockTower || room == TowerPassage))
        return false;
    struct { uint8_t room, flags; uint16_t reserved; } layout = {0};
    if (streamed && (!corridor || fread(&layout, sizeof layout, 1, file) != 1 ||
                     layout.room != room || layout.reserved ||
                     (layout.flags & ~(RoomEdgeHeights | RoomFloorPlanes | RoomWideVertices | RoomGroundPlanes | RoomBoilerEvent | RoomDoorNotice | RoomLadderAscent)) ||
                     ((layout.flags & RoomLadderAscent) && (!hasOrigin || room != TowerPassage)) ||
                     ((layout.flags & RoomDoorNotice) && (!hasOrigin || room != ClockTower)) ||
                     ((layout.flags & RoomBoilerEvent) && (!hasOrigin || room != BoilerRoom)) ||
                     ((layout.flags & RoomGroundPlanes) && (!hasOrigin || !(layout.flags & RoomFloorPlanes)))))
        return false;
    RoomOrigin origin;
    if (hasOrigin) {
        int32_t q8[2];
        // Allow room for signed Q12 translations and local wall coordinates.
        const int limit = INT32_MAX/32;
        if (fread(q8, sizeof q8, 1, file) != 1 ||
            q8[0] < -limit || q8[0] > limit || q8[1] < -limit || q8[1] > limit)
            return false;
        origin = (RoomOrigin){16*q8[0], 16*q8[1]};
    } else {
        origin = legacyRoomOrigin(room);
    }
    bool firstFloorObstacles = room == FirstFloor && !memcmp(header.magic, "SHF3", 4);
    bool southHallObstacles = room == SouthHall && !memcmp(header.magic, "SHS2", 4);
    bool countedFirstFloor = room == FirstFloor && !memcmp(header.magic, "SHF4", 4);
    bool countedEast = room == EastCorridor && !memcmp(header.magic, "SHE2", 4);
    bool heightEast = room == EastCorridor && !memcmp(header.magic, "SHE3", 4);
    EdgeLayout edgeLayout = LegacyEdges;
    if (heightEast)
        edgeLayout = HeightEdges;
    else if (room == Courtyard || countedFirstFloor || countedEast)
        edgeLayout = CountedEdges;
    else if (firstFloorObstacles || southHallObstacles)
        edgeLayout = ExpandedEdges;
    FloorLayout floorLayout = NoFloorData;
    if (streamed) {
        magic = hasOrigin ? "SHM2" : "SHM1";
        edgeLayout = layout.flags & RoomEdgeHeights ? HeightEdges : CountedEdges;
        if (layout.flags & RoomFloorPlanes)
            floorLayout = layout.flags & RoomGroundPlanes ? CountedGroundPlanes : CountedFloorPlanes;
    } else if (room == Courtyard) {
        magic = "SHY1";
        floorLayout = LegacyWalkways;
        if (!memcmp(header.magic, "SHY3", 4)) {
            magic = "SHY3";
            floorLayout = CountedFloorPlanes;
        } else if (!memcmp(header.magic, "SHY2", 4)) {
            magic = "SHY2";
            floorLayout = LegacyCourtyardFloors;
        }
    }
    bool boiler = (layout.flags & RoomBoilerEvent) != 0;
    bool latchText = countedFirstFloor || firstFloorObstacles ||
                     (room == FirstFloor && !memcmp(header.magic, "SHF2", 4));
    bool medical = room == Infirmary && !memcmp(header.magic, "SHI3", 4);
    bool chemical = room == EquipmentRoom && !memcmp(header.magic, "SHQ2", 4);
    bool hand = room == ChemistryLab && !memcmp(header.magic, "SHL2", 4);
    bool pour = room == ChemistryLab && !memcmp(header.magic, "SHL3", 4);
    bool infirmaryItems = medical || (room == Infirmary && !memcmp(header.magic, "SHI2", 4));
    enum { KeyTileBytes = 64*64/2, SmokeTileBytes = 32*128 };
    unsigned itemPages = pour ? 2 : chemical || infirmaryItems ? 1 : 0;
    unsigned itemBytes = pour ? KeyTileBytes+SmokeTileBytes : chemical ? KeyTileBytes :
                         infirmaryItems ? 64*256/2 : 0;
    unsigned itemOffset = 128*1024-itemBytes;
    if (header.textureBytes < itemBytes) return false;
    unsigned sceneBytes = header.textureBytes - itemBytes;
    unsigned scenePages = streamed ? header.pages :
                          room == SchoolCorridor || room == UpperNorthHall || room == ChemistryLab ? 2 : 3;
    if (countedFirstFloor) magic = "SHF4";
    else if (latchText) magic = firstFloorObstacles ? "SHF3" : "SHF2";
    if (heightEast) magic = "SHE3";
    else if (countedEast) magic = "SHE2";
    if (southHallObstacles) magic = "SHS2";
    if (infirmaryItems) magic = medical ? "SHI3" : "SHI2";
    if (chemical) magic = "SHQ2";
    if (hand) magic = "SHL2";
    if (pour) magic = "SHL3";
    if ((corridor ? (memcmp(header.magic, magic, 4) || header.pages != scenePages + itemPages || !playerReady() ||
                     sceneBytes > skin.offset || header.paletteBytes > skin.firstPalette*32u) :
         !((!memcmp(header.magic, "SHR1", 4) && header.pages == 3) ||
           (!memcmp(header.magic, "SHR", 3) && header.magic[3] >= '2' && header.magic[3] <= '5' && header.pages == 4))) ||
        !header.pages || header.pages > sizeof pages/sizeof pages[0] ||
        header.batches > MaxBatches || !header.batches ||
        header.textureBytes > 128*1024 || (header.textureBytes & 7) ||
        header.paletteBytes > 64*1024 || (header.paletteBytes & 31) ||
        fread(pages, sizeof(Page), header.pages, file) != header.pages)
        return false;
    for (unsigned i = 0; i < header.pages; i++) {
        const Page *p = &pages[i];
        bool atlas = itemPages && i >= scenePages;
        bool smoke = pour && i == scenePages+1;
        if (atlas && (p->width != (smoke ? 32 : 64) ||
            p->offset != sceneBytes+(smoke ? KeyTileBytes : 0) ||
            p->palettes != (smoke ? 2u : infirmaryItems ? 4u : 1u)))
            return false;
        unsigned limit = atlas ? header.textureBytes : sceneBytes;
        unsigned pageBytes = smoke ? SmokeTileBytes : atlas && !infirmaryItems ? KeyTileBytes : p->width*128u;
        if ((!atlas && p->width != 128 && p->width != 256) || (p->offset & 7) ||
            p->offset > limit || pageBytes > limit-p->offset ||
            p->firstPalette > header.paletteBytes/32 ||
            p->palettes > header.paletteBytes/32 - p->firstPalette)
            return false;
    }
    // On Continue, the saved room will provide its own scenery textures.
    unsigned textureStart = !scenery && header.pages == 4 ? pages[3].offset : 0;
    if (textureStart && fseek(file, textureStart, SEEK_CUR)) return false;
    vramSetBankC(VRAM_C_LCD);
    vramSetBankE(VRAM_E_LCD);
    if (!upload(file, (uint8_t *)VRAM_C + textureStart, sceneBytes-textureStart)) return false;
    if (itemPages) {
        // Keep Harry and his silver pickup resident in texture bank C.
        if (skin.offset + 256*256/2 + 2048 > itemOffset ||
            !upload(file, (uint8_t *)VRAM_C + itemOffset, itemBytes)) return false;
        for (unsigned i = scenePages; i < header.pages; i++) pages[i].offset += itemOffset-sceneBytes;
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
        !collisionLoad(file, room, edgeLayout, floorLayout, origin)) return false;
    if (latchText && !latchLoad(file)) return false;
    if (boiler && !boilerLoad(file)) return false;
    if ((layout.flags & RoomDoorNotice) && !towerDoorLoad(file)) return false;
    if (room == TowerPassage && !playerLoadLadder(file, layout.flags & RoomLadderAscent)) return false;
    if (room == MusicRoom && header.pages == 4 &&
        (pages[3].width != 256 || pages[3].palettes != 15 || !playerLoad(file, header.magic[3]-'0')))
        return false;

    if (header.magic[3] == '5' &&
        (header.textureBytes < 2048 || header.paletteBytes < 32 ||
         !pickupLoad(file, SilverMedallion, header.textureBytes-2048, header.paletteBytes/32-1)))
        return false;
    if (medical && (!pickupLoad(file, HealthDrink, itemOffset, pages[scenePages].firstPalette+1) ||
                    !pickupLoad(file, FirstAidKit, itemOffset, pages[scenePages].firstPalette)))
        return false;
    if (chemical && (!pickupLoad(file, Chemical, itemOffset, pages[scenePages].firstPalette) ||
                     !pickupLoadEmptyShelf(file)))
        return false;
    if (pour && (!playerLoadPour(file) ||
                 !statueLoad(file, pages[scenePages+1].offset, pages[scenePages+1].firstPalette) ||
                 !pickupLoad(file, GoldMedallion, itemOffset, pages[scenePages].firstPalette) ||
                 !pickupLoadEmptyHand(file)))
        return false;

    // Derive the command buffer size from the validated on-card vertex count.
    long start = ftell(file);
    if (start < 0) return false;
    long endOfFile = roomFileSize(file, start);
    long batchHeaders = header.batches * sizeof(Batch);
    if (endOfFile < start || endOfFile-start < batchHeaders) return false;
    long bytes = endOfFile - start - batchHeaders;
    unsigned maxVerts = streamed || room == Courtyard ? MaxRoomVertices :
                        room == MusicRoom || room == SchoolCorridor ? 6144 : 8192;
    if (bytes <= 0 || bytes > maxVerts*(long)sizeof(Vertex) || bytes % (4*sizeof(Vertex)))
        return false;
    if (!scenery) return true;
    unsigned pos = 0, total = bytes / sizeof(Vertex);
    // Two full vertices need six parameters and one packed command word.
    size_t geometryBytes = total/2 * 7 * sizeof(*geometry);
    free(geometry);
    geometry = malloc(geometryBytes);
    if (!geometry) return false;
    unsigned maxState = boiler ? BoilerGlowBatch : pour ? GoldAtHandBatch : chemical ? ChemicalBatch : medical ? KitBatch :
                        corridor ? StaticBatch : FallenSilverBatch;
    uint32_t *end = geometry;
    GeometryChunk *chunk = chunks;
    for (unsigned i = 0; i < header.batches; i++) {
        Batch *b = &batches[i];
        if (fread(b, sizeof *b, 1, file) != 1 || b->page >= header.pages ||
            b->state > maxState || b->flags > (streamed || room == Courtyard ? 7 : 3) ||
            b->palette >= pages[b->page].palettes ||
            ((chemical || pour) && b->page >= scenePages) ||
            b->quads > (total-pos)/4)
            return false;
        uint32_t *next = loadGeometry(file, end, b->quads, &chunk);
        if (!next) return false;
        batchWords[i] = next-end;
        end = next;
        pos += 4*b->quads;
    }
    if (pos != total) return false;
    geometryBytes = (end-geometry)*sizeof(*geometry);
    uint32_t *compact = realloc(geometry, geometryBytes);
    if (compact) geometry = compact;
    DC_FlushRange(geometry, geometryBytes);
    visibilityValid = false;
    currentOrigin = origin;
    current = room;
    wideGeometry = streamed ? (layout.flags & RoomWideVertices) != 0 :
                   room == EastCorridor || room == FirstFloor || room == NorthHall;
    handPresent = hand || pour;
    handPuzzle = pour;
    return true;
}

bool roomLoad(FILE *file, SchoolRoom room)
{
    return loadRoom(file, room, true);
}

bool roomLoadShared(FILE *file)
{
    return loadRoom(file, MusicRoom, false);
}

static bool updateRoomFrustum(ClipPlane planes[6], int scale)
{
    int clip[16];
    glGetFixed(GL_GET_MATRIX_CLIP, clip);
    if (visibilityValid && !memcmp(clip, visibilityMatrix, sizeof clip)) return false;
    memcpy(visibilityMatrix, clip, sizeof clip);
    visibilityValid = true;
    for (unsigned axis = 0; axis < 3; axis++) {
        ClipPlane *low = &planes[2*axis], *high = low+1;
        for (unsigned i = 0; i < 3; i++) {
            low->normal[i] = scale*(clip[4*i+3]+clip[4*i+axis]);
            high->normal[i] = scale*(clip[4*i+3]-clip[4*i+axis]);
            low->corner[i] = i + (low->normal[i] < 0 ? 0 : 3);
            high->corner[i] = i + (high->normal[i] < 0 ? 0 : 3);
        }
        low->offset = ((int64_t)clip[15]+clip[12+axis])*4096;
        high->offset = ((int64_t)clip[15]-clip[12+axis])*4096;
    }
    return true;
}

static ARM_CODE inline bool chunkVisible(const GeometryChunk *chunk, const ClipPlane planes[6])
{
    const int16_t *bounds = chunk->bounds;
    for (unsigned i = 0; i < 6; i++) {
        const ClipPlane *p = &planes[i];
        int64_t distance = p->offset
                         + (int64_t)p->normal[0]*bounds[p->corner[0]]
                         + (int64_t)p->normal[1]*bounds[p->corner[1]]
                         + (int64_t)p->normal[2]*bounds[p->corner[2]];
        // Leave room for the geometry engine's fixed-point rounding.
        if (distance < -8*4096) return false;
    }
    return true;
}

static inline void submitGeometry(const uint32_t *commands, unsigned words)
{
    // Commands are flushed at load. Finish the DMA before the next GPU write.
    while (dmaBusy(0)) {}
    dmaSetParams(0, commands, (void *)&GFX_FIFO, DMA_FIFO | words);
    while (dmaBusy(0)) {}
}

static ARM_CODE void submitVisible(const uint32_t *commands, const GeometryChunk *chunk,
                                   unsigned count, bool *inView, const ClipPlane *updated)
{
    unsigned run = 0;
    for (unsigned i = 0; i < count; i++) {
        if (updated) inView[i] = chunkVisible(&chunk[i], updated);
        if (inView[i]) {
            run += chunk[i].words;
        } else {
            if (run) submitGeometry(commands, run);
            commands += run + chunk[i].words;
            run = 0;
        }
    }
    if (run) submitGeometry(commands, run);
}

void roomDraw(const Medallion *medal)
{
    bool solved = medal->phase >= MedalLanded;
    bool gameplay = playerReady() && (medal->phase == MedalHanging || medal->phase == MedalFinished);
    // MAP1_S01 medallion cameras. Local origin (-100, 0, 20), 2 metres/unit.
    // Screen_CutsceneCameraStateUpdate masks the 320x240 picture at y=±96.
    // Shrink the frustum with the viewport to keep the original pixel scale.
    int border = statueActive() ? (int)statueBorder() : gameplay ? 6 : 19;
    glViewport(0, border, 255, 191-border);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const int distance = statueCamera() ? 376 : gameplay ? 224 :
                         medal->phase == MedalHanging ? 353 : medal->projection;
    const int near = 1024; // 0.25 scene units, Q12.
    int horizontal = 160 * near / distance;
    int vertical = (120-(5*border+2)/4)*near/distance;
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
    const uint32_t *commands = geometry;
    const GeometryChunk *chunk = chunks;
    bool *inView = visibleChunks;
    ClipPlane planes[6];
    // Only the room geometry gets the additional scale, applied below.
    const ClipPlane *updated = updateRoomFrustum(planes, wideGeometry ? 2 : 1) ? planes : NULL;
    if (playerReady()) {
        GFX_TEX_FORMAT = (skin.offset >> 3) | (TEXTURE_SIZE_256 << 20)
                         | (TEXTURE_SIZE_256 << 23) | (GL_RGB16 << 26);
        playerDraw(skin.firstPalette);
    }
    if (wideGeometry) glScalef32(8192, 8192, 8192);
    bool visible[] = {
        [StaticBatch] = true,
        [HangingSilverBatch] = !solved,
        [FallenSilverBatch] = solved && !pickupHidden(SilverMedallion),
        [DrinkBatch] = !pickupHidden(HealthDrink),
        [KitBatch] = !pickupHidden(FirstAidKit),
        [ChemicalBatch] = !pickupHidden(Chemical),
        [IntactHandBatch] = !statueOpened(),
        [OpenHandBatch] = statueOpened(),
        [GoldAtHandBatch] = statueOpened() && !pickupHidden(GoldMedallion),
        [BoilerUnwiredBatch] = !(towerFlags() & MoonPlaced),
        [BoilerReadyBatch] = (towerFlags() & MoonPlaced) != 0,
        [BoilerGlowBatch] = boilerGlow(),
    };
    for (unsigned i = 0; i < header.batches; i++) {
        const Batch *b = &batches[i];
        unsigned count = batchWords[i];
        unsigned chunkCount = (b->quads+ChunkQuads-1)/ChunkQuads;
        if (count && visible[b->state]) {
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
            if (b->state == StaticBatch)
                submitVisible(commands, chunk, chunkCount, inView, updated);
            else
                submitGeometry(commands, count);
            glEnd();
            if (moving) glPopMatrix(1);
        }
        commands += count;
        chunk += chunkCount;
        inView += chunkCount;
    }
    statueDrawSmoke();
    pickupDrawItem();
    glFlush(0);
}
