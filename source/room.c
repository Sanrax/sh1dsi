#include <nds.h>
#include <string.h>
#include <sys/stat.h>
#include "room.h"
#include "player.h"
#include "collision.h"
#include "pickup.h"
#include "statue.h"
#include "otherworld.h"
#include "lighting.h"
#include "phone.h"

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
       IntactHandBatch, OpenHandBatch, GoldAtHandBatch, BoilerUnwiredBatch, BoilerReadyBatch, BoilerGlowBatch,
       LobbyKitBatch, LobbyBulletsBatch, LobbyNearFanBatch, LobbyFarFanBatch, PictureCardBatch, PhoneDeskBatch };

typedef RoomVertex Vertex;
_Static_assert(sizeof(LitPoint) == 10 && sizeof(LitCorner) == 4, "lit room vertex layout");

_Static_assert(sizeof(Vertex) == 8, "room vertex layout");

static struct {
    char magic[4];
    uint16_t pages, batches;
    uint32_t textureBytes, paletteBytes;
} header;
static Page pages[4], skin;
enum { RoomEdgeHeights = 1, RoomFloorPlanes = 2, RoomWideVertices = 4, RoomGroundPlanes = 8, RoomBoilerEvent = 16, RoomDoorNotice = 32, RoomLadderAscent = 64, RoomNormals = 128 };
static SchoolRoom current = MusicRoom;
static RoomOrigin currentOrigin = {-100*4096, 20*4096};
static bool handPresent, handPuzzle, wideGeometry;
static unsigned fanAngle[2];

// MAP1_S00–S02 door points. Each adjacent pair links two doors.
// Normals point into the room.
static const Door schoolDoors[] = {
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
    {OtherworldCourtyard, OtherworldNorthHall, -80691, -215040, 0, -1, 4864},
    {OtherworldNorthHall, OtherworldCourtyard, 546816, 771276, 0, 1, 4864},
    {OtherworldNorthHall, OtherworldLobby, 546816, 790937, 0, -1, 4864},
    {OtherworldLobby, OtherworldNorthHall, 409600, 560332, 0, 1, 4864},
    {OtherworldNorthHall, OtherworldSideHall, 564838, 790937, 0, -1, 2304},
    {OtherworldSideHall, OtherworldNorthHall, 543948, 560332, 0, 1, 2304},
    {OtherworldSideHall, OtherworldEastHall, 570163, 560332, 0, 1, 2304},
    {OtherworldEastHall, OtherworldSideHall, 441548, 299417, 0, -1, 2304},
    {OtherworldEastHall, NorthClassroom, 449740, 294502, -1, 0, 2304},
    {NorthClassroom, OtherworldEastHall, 556236, 431718, 1, 0, 2304},
    {OtherworldEastHall, NorthClassroom, 449740, 251904, -1, 0, 2304},
    {NorthClassroom, OtherworldEastHall, 556236, 387481, 1, 0, 2304},
    {NorthClassroom, SouthClassroom, 578355, 383385, 0, 1, 2304},
    {SouthClassroom, NorthClassroom, 586547, 271974, 0, -1, 2304},
    {SouthClassroom, OtherworldEastHall, 564428, 223641, 1, 0, 2304},
    {OtherworldEastHall, SouthClassroom, 449740, 199475, -1, 0, 2304},
    {OtherworldEastHall, OtherworldSouthHall, 436633, 201113, 1, 0, 3584},
    {OtherworldSouthHall, OtherworldEastHall, 612761, 81920, -1, 0, 3584},
    {OtherworldSouthHall, OtherworldReception, 539033, 65536, 0, 1, 2304},
    {OtherworldReception, OtherworldSouthHall, 234291, 87244, 0, -1, 2304},
    {OtherworldReception, OtherworldWestHall, 229376, 79052, 1, 0, 2304},
    {OtherworldWestHall, OtherworldReception, 255590, 173670, -1, 0, 2304},
    // MAP1_S02 points 83/87, 84/88, 86/91 and 89/92.
    {OtherworldWestHall, TeachersSouth, 242483, 196608, 1, 0, 2304},
    {TeachersSouth, OtherworldWestHall, 90931, 391577, -1, 0, 2304},
    {OtherworldWestHall, TeachersSouth, 242483, 239206, 1, 0, 2304},
    {TeachersSouth, OtherworldWestHall, 90931, 435814, -1, 0, 2304},
    {OtherworldWestHall, TeachersNorth, 242483, 292044, 1, 0, 2304},
    {TeachersNorth, OtherworldWestHall, 95027, 595558, -1, 0, 2304},
    {TeachersSouth, TeachersNorth, 68812, 439910, 0, -1, 2304},
    {TeachersNorth, TeachersSouth, 72908, 547225, 0, 1, 2304},
    {BasementStair, BoilerRoom, 219545, -70451, 1, 0, 2304},
    {BoilerRoom, BasementStair, 93388, -79872, -1, 0, 2304}
};
_Static_assert((sizeof schoolDoors/sizeof schoolDoors[0]) % 2 == 0, "paired room doors");
enum { PairedDoorCount = sizeof schoolDoors/sizeof schoolDoors[0] };

// MAP1_S02 and MAP1_S03 points 81, 9, 58 and 10 form a directed
// route. The restroom uses one chunk on both overlays, but its exit
// depends on which hall Harry entered from. Each arrival uses the next door.
static const Door restroomDoors[] = {
    {OtherworldWestHall, OtherworldUpperRestroom, 242483, 170393, 1, 0, 2304},
    {OtherworldUpperRestroom, OtherworldUpperWestHall, 95313, 81510, -1, 0, 2304},
    {OtherworldUpperWestHall, OtherworldRestroom, -408780, 168755, 1, 0, 2304},
    {OtherworldRestroom, OtherworldWestHall, 95313, 81510, -1, 0, 2304}
};
enum { RestroomDoorCount = sizeof restroomDoors/sizeof restroomDoors[0] };
const unsigned roomDoorCount = PairedDoorCount + RestroomDoorCount;

const Door *roomDoor(unsigned index)
{
    return index < PairedDoorCount ? &schoolDoors[index] : &restroomDoors[index-PairedDoorCount];
}

const Door *roomDoorArrival(unsigned index)
{
    return index < PairedDoorCount ? &schoolDoors[index ^ 1] :
           &restroomDoors[(index-PairedDoorCount+1)%RestroomDoorCount];
}

SchoolRoom roomCurrent(void) { return current; }
bool roomHasHand(void) { return current == ChemistryLab && handPresent; }
bool roomCanDissolveHand(void) { return current == ChemistryLab && handPuzzle; }
RoomOrigin roomOrigin(void) { return currentOrigin; }

void roomTick(void)
{
    if (current != OtherworldLobby) return;
    // The two MAP1_S02 fans vary their angular speeds independently.
    static uint32_t rng;
    for (unsigned fan = 0; fan < 2; fan++) {
        rng = rng*1664525u + 1013904223u;
        unsigned speed = (fan ? 2332 : 2048) + ((rng >> 17) & 511);
        // Rotation about Z changes sign in the DS room basis.
        fanAngle[fan] = (fanAngle[fan] + ((speed*136) >> 12)) & 4095;
    }
}

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
    case OtherworldCourtyard:
    case OtherworldNorthHall:
    case OtherworldLobby:
    case OtherworldSideHall:
    case OtherworldEastHall:
    case OtherworldSouthHall:
    case OtherworldReception:
    case OtherworldWestHall:
    case OtherworldRestroom:
    case OtherworldUpperRestroom:
    case OtherworldUpperWestHall:
    case TeachersSouth:
    case TeachersNorth:
    case NorthClassroom:
    case SouthClassroom:
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
static LitCorner *litGeometry;
static LitPoint *lightPoints;
static uint16_t *lightColors;
static uint16_t lightCount;

bool roomHasNormals(void) { return litGeometry != NULL; }

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

static bool loadLitGeometry(FILE *file, LitCorner *vertices, unsigned count, GeometryChunk **chunk)
{
    if (fread(vertices, sizeof *vertices, count, file) != count) return false;
    for (unsigned i = 0; i < count; i++)
        if (vertices[i].point >= lightCount) return false;
    for (unsigned i = 0; i < count;) {
        GeometryChunk *part = (*chunk)++;
        unsigned size = count-i < 4*ChunkQuads ? count-i : 4*ChunkQuads;
        unsigned end = i+size;
        const LitPoint *first = &lightPoints[vertices[i].point];
        part->bounds[0] = part->bounds[3] = first->x;
        part->bounds[1] = part->bounds[4] = first->y;
        part->bounds[2] = part->bounds[5] = first->z;
        part->words = size;
        for (; i < end; i++) {
            const LitPoint *v = &lightPoints[vertices[i].point];
            if (v->x < part->bounds[0]) part->bounds[0] = v->x;
            if (v->x > part->bounds[3]) part->bounds[3] = v->x;
            if (v->y < part->bounds[1]) part->bounds[1] = v->y;
            if (v->y > part->bounds[4]) part->bounds[4] = v->y;
            if (v->z < part->bounds[2]) part->bounds[2] = v->z;
            if (v->z > part->bounds[5]) part->bounds[5] = v->z;
        }
    }
    return true;
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
    bool lobby = room == OtherworldLobby;
    bool pictureCard = room == NorthClassroom;
    bool otherworld = room == OtherworldCourtyard || room == OtherworldNorthHall || lobby ||
                      room == OtherworldSideHall || room == OtherworldEastHall ||
                      room == OtherworldSouthHall || room == OtherworldReception || room == OtherworldWestHall ||
                      pictureCard || room == SouthClassroom || roomIsRestroom(room) ||
                      room == OtherworldUpperWestHall || roomIsTeachersRoom(room);
    if (!hasOrigin && (room == BasementStair || room == BoilerRoom ||
                       room == ClockTower || room == FarClockTower || room == TowerPassage ||
                       otherworld))
        return false;
    // The two overlays load the same physical restroom geometry.
    SchoolRoom streamRoom = roomIsRestroom(room) ? OtherworldRestroom : room;
    struct { uint8_t room, flags; uint16_t reserved; } layout = {0};
    if (streamed && (!corridor || fread(&layout, sizeof layout, 1, file) != 1 ||
                     layout.room != streamRoom || layout.reserved ||
                     (layout.flags & ~(RoomEdgeHeights | RoomFloorPlanes | RoomWideVertices | RoomGroundPlanes | RoomBoilerEvent | RoomDoorNotice | RoomLadderAscent | RoomNormals)) ||
                     ((layout.flags & RoomNormals) && (!hasOrigin || !otherworld)) ||
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
    enum { KeyTileBytes = 64*64/2, SmokeTileBytes = 32*128, SupplyTileBytes = 64*128/2 };
    bool phoneRoom = room == TeachersNorth;
    unsigned itemPages = pour ? 2 : chemical || infirmaryItems || lobby || pictureCard || phoneRoom ? 1 : 0;
    unsigned itemBytes = pour ? KeyTileBytes+SmokeTileBytes : (chemical || pictureCard || phoneRoom) ? KeyTileBytes :
                         infirmaryItems ? 64*256/2 : lobby ? SupplyTileBytes : 0;
    unsigned itemOffset = 128*1024-itemBytes;
    if (header.textureBytes < itemBytes) return false;
    unsigned sceneBytes = header.textureBytes - itemBytes;
    if (header.pages <= itemPages) return false;
    unsigned scenePages = streamed ? header.pages-itemPages :
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
            p->palettes != (smoke || lobby ? 2u : infirmaryItems ? 4u : 1u)))
            return false;
        unsigned limit = atlas ? header.textureBytes : sceneBytes;
        unsigned pageBytes = smoke ? SmokeTileBytes : atlas && lobby ? SupplyTileBytes :
                             atlas && !infirmaryItems ? KeyTileBytes : p->width*128u;
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
    if (room == OtherworldCourtyard && !otherworldLoadIntro(file, current == FarClockTower))
        return false;
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
    if (lobby && (!pickupLoad(file, LobbyKit, itemOffset, pages[scenePages].firstPalette) ||
                  !pickupLoad(file, LobbyBullets, itemOffset, pages[scenePages].firstPalette+1) ||
                  !lobbyFanLoad(file)))
        return false;
    if (pictureCard && (!pickupLoad(file, PictureCard, itemOffset, pages[scenePages].firstPalette) ||
                        !pickupLoadScatteredCards(file)))
        return false;
    if (chemical && (!pickupLoad(file, Chemical, itemOffset, pages[scenePages].firstPalette) ||
                     !pickupLoadEmptyShelf(file)))
        return false;
    if (pour && (!playerLoadPour(file) ||
                 !statueLoad(file, pages[scenePages+1].offset, pages[scenePages+1].firstPalette) ||
                 !pickupLoad(file, GoldMedallion, itemOffset, pages[scenePages].firstPalette) ||
                 !pickupLoadEmptyHand(file)))
        return false;

    bool lit = layout.flags & RoomNormals;
    if (scenery) {
        free(lightPoints);
        free(lightColors);
        lightPoints = NULL;
        lightColors = NULL;
    }
    if (lit) {
        if (fread(&lightCount, sizeof lightCount, 1, file) != 1 ||
            !lightCount || lightCount > MaxRoomVertices)
            return false;
        lightPoints = malloc(lightCount*sizeof *lightPoints);
        lightColors = malloc(lightCount*sizeof *lightColors);
        if (!lightPoints || !lightColors ||
            fread(lightPoints, sizeof *lightPoints, lightCount, file) != lightCount)
            return false;
        for (unsigned i = 0; i < lightCount; i++)
            if (lightPoints[i].reserved) return false;
    }

    // Derive the command buffer size from the validated on-card vertex count.
    long start = ftell(file);
    if (start < 0) return false;
    long endOfFile = roomFileSize(file, start);
    long batchHeaders = header.batches * sizeof(Batch);
    if (endOfFile < start || endOfFile-start < batchHeaders) return false;
    long bytes = endOfFile - start - batchHeaders;
    unsigned maxVerts = streamed || room == Courtyard ? MaxRoomVertices :
                        room == MusicRoom || room == SchoolCorridor ? 6144 : 8192;
    size_t stride = lit ? sizeof(LitCorner) : sizeof(Vertex);
    if (bytes <= 0 || bytes > maxVerts*(long)stride || bytes % (4*stride))
        return false;
    if (!scenery) return true;
    unsigned pos = 0, total = bytes / stride;
    // Two full vertices need six parameters and one packed command word.
    size_t geometryBytes = total/2 * 7 * sizeof(*geometry);
    free(geometry);
    free(litGeometry);
    geometry = lit ? NULL : malloc(geometryBytes);
    litGeometry = lit ? malloc(total*sizeof *litGeometry) : NULL;
    if (!geometry && !litGeometry) return false;
    unsigned maxState = phoneRoom ? PhoneDeskBatch : pictureCard ? PictureCardBatch : lobby ? LobbyFarFanBatch : boiler ? BoilerGlowBatch : pour ? GoldAtHandBatch : chemical ? ChemicalBatch : medical ? KitBatch :
                        corridor ? StaticBatch : FallenSilverBatch;
    uint32_t *end = geometry;
    GeometryChunk *chunk = chunks;
    for (unsigned i = 0; i < header.batches; i++) {
        Batch *b = &batches[i];
        if (fread(b, sizeof *b, 1, file) != 1 || b->page >= header.pages ||
            b->state > maxState || b->flags > (streamed || room == Courtyard ? 7 : 3) ||
            b->palette >= pages[b->page].palettes ||
            ((chemical || pour || phoneRoom) && b->page >= scenePages) ||
            (phoneRoom && b->state != StaticBatch && b->state != PhoneDeskBatch) ||
            b->quads > (total-pos)/4)
            return false;
        if (lit) {
            if (!loadLitGeometry(file, litGeometry+pos, 4*b->quads, &chunk)) return false;
            batchWords[i] = 4*b->quads;
        } else {
            uint32_t *next = loadGeometry(file, end, b->quads, &chunk);
            if (!next) return false;
            batchWords[i] = next-end;
            end = next;
        }
        pos += 4*b->quads;
    }
    if (pos != total) return false;
    if (lit) {
        lightingLoadRoom(current == FarClockTower);
    } else {
        geometryBytes = (end-geometry)*sizeof(*geometry);
        uint32_t *compact = realloc(geometry, geometryBytes);
        if (compact) geometry = compact;
        DC_FlushRange(geometry, geometryBytes);
    }
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

// Vertices share shaded positions and normals, with independent texture coordinates.
static ARM_CODE void submitLitGeometry(const LitCorner *corners, unsigned count)
{
    static uint32_t commands[5*4*ChunkQuads] __attribute__((aligned(32)));
    uint32_t *out = commands;
    for (unsigned i = 0; i < count; i++) {
        const LitCorner *corner = &corners[i];
        const LitPoint *point = &lightPoints[corner->point];
        *out++ = FIFO_COMMAND_PACK(FIFO_COLOR, FIFO_TEX_COORD, FIFO_VERTEX16, 0);
        *out++ = lightColors[corner->point];
        *out++ = ((uint32_t)corner->v << 20) | ((uint32_t)corner->u << 4);
        *out++ = (uint16_t)point->x | ((uint32_t)(uint16_t)point->y << 16);
        *out++ = (uint32_t)(int32_t)point->z;
    }
    unsigned words = out-commands;
    DC_FlushRange(commands, words*sizeof(*commands));
    submitGeometry(commands, words);
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
    bool intro = otherworldIntroActive(), slot = slotActive(), phone = phoneActive();
    int border = statueActive() ? (int)statueBorder() : phone ? phoneBorder() :
                 gameplay && !intro && !slot ? 6 : 19;
    glViewport(0, border, 255, 191-border);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const int distance = intro ? otherworldIntroProjection() : phone ? phoneProjection() :
                         statueCamera() || slot ? 376 : gameplay ? 224 :
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
    unsigned fog = litGeometry ? lightingFog() : 0;
    if (!litGeometry) glDisable(GL_FOG);
    glClearColor(0, 0, 0, 31);
    glClearPolyID(63);
    glClearDepth(0x7fff);
    glColor(litGeometry ? lightingActorColor() : RGB15(31,31,31));
    Flashlight lamp = {0};
    const LitCorner *lit = litGeometry;
    const uint32_t *commands = geometry;
    const GeometryChunk *chunk = chunks;
    bool *inView = visibleChunks;
    ClipPlane planes[6];
    // Only the room geometry gets the additional scale, applied below.
    const ClipPlane *updated = updateRoomFrustum(planes, wideGeometry ? 2 : 1) ? planes : NULL;
    if (playerReady()) {
        GFX_TEX_FORMAT = (skin.offset >> 3) | (TEXTURE_SIZE_256 << 20)
                         | (TEXTURE_SIZE_256 << 23) | (GL_RGB16 << 26);
        unsigned receiverTexture = 0, receiverPalette = 0;
        if (phoneHolding()) {
            const Page *receiver = &pages[header.pages-1];
            receiverTexture = (receiver->offset >> 3) | (TEXTURE_SIZE_64 << 20) |
                              (TEXTURE_SIZE_64 << 23) | (GL_RGB16 << 26);
            receiverPalette = receiver->firstPalette;
        }
        playerDraw(skin.firstPalette, fog, lit ? &lamp : NULL, receiverTexture, receiverPalette);
    }
    if (lit) lightingShade(lightPoints, lightColors, lightCount, &lamp, wideGeometry ? 2 : 3);
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
        [LobbyKitBatch] = !pickupHidden(LobbyKit),
        [LobbyBulletsBatch] = !pickupHidden(LobbyBullets),
        [LobbyNearFanBatch] = true,
        [LobbyFarFanBatch] = true,
        [PictureCardBatch] = !pickupHidden(PictureCard),
        [PhoneDeskBatch] = phoneDeskVisible(),
    };
    for (unsigned i = 0; i < header.batches; i++) {
        const Batch *b = &batches[i];
        unsigned count = batchWords[i];
        unsigned chunkCount = (b->quads+ChunkQuads-1)/ChunkQuads;
        if (count && visible[b->state]) {
            bool spinning = b->state == LobbyNearFanBatch || b->state == LobbyFarFanBatch;
            bool moving = b->state == HangingSilverBatch && medal->phase >= MedalSwing;
            if (moving) {
                // SHR1 stores the hanging model with its Q8 pivot baked in.
                glPushMatrix();
                glTranslatef32(2048, -medal->y / 2, (20 * 4096 - medal->z) / 2);
                glRotateXi(medal->angle * 8);
                glTranslatef32(-2048, -5488, 7640);
            } else if (spinning) {
                // Pivots in Q8 relative to the (100, 140) lobby origin.
                unsigned fan = b->state-LobbyNearFanBatch;
                int depth = fan ? 1344*8 : 742*8;
                glPushMatrix();
                glTranslatef32(0, 410*8, -depth);
                glRotateZi(fanAngle[fan]*8);
                glTranslatef32(0, -410*8, depth);
            }
            const Page *p = &pages[b->page];
            unsigned width = p->width == 64 ? TEXTURE_SIZE_64 :
                             p->width == 128 ? TEXTURE_SIZE_128 : TEXTURE_SIZE_256;
            GFX_TEX_FORMAT = (b->flags & 4) ? 0 :
                             (p->offset >> 3) | (width << 20) | (TEXTURE_SIZE_256 << 23)
                             | (GL_RGB16 << 26) | ((b->flags & 2) ? GL_TEXTURE_COLOR0_TRANSPARENT : 0);
            GFX_PAL_FORMAT = (p->firstPalette + b->palette) * 2;
            // PS1 visible faces wind clockwise in these screen coordinates.
            glPolyFmt(POLY_ALPHA((b->flags & 1) ? 16 : 31) | POLY_CULL_FRONT | fog);
            glBegin(GL_QUADS);
            if (lit) {
                const LitCorner *vertices = lit;
                for (unsigned j = 0; j < chunkCount; j++) {
                    if (updated) inView[j] = spinning || chunkVisible(&chunk[j], updated);
                    if (inView[j]) submitLitGeometry(vertices, chunk[j].words);
                    vertices += chunk[j].words;
                }
            } else if (b->state == StaticBatch)
                submitVisible(commands, chunk, chunkCount, inView, updated);
            else
                submitGeometry(commands, count);
            glEnd();
            if (moving || spinning) glPopMatrix(1);
        }
        if (lit) lit += count;
        else commands += count;
        chunk += chunkCount;
        inView += chunkCount;
    }
    statueDrawSmoke();
    pickupDrawItem();
    glFlush(litGeometry ? GL_WBUFFERING : 0);
}
