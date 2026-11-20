#include "checkpoint.h"
#include "room.h"
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    char magic[4];
    uint32_t generation;
    int32_t x, z;
    uint32_t ticks, saves;
    uint16_t yaw;
    uint8_t room, legacyFlags;
    uint16_t flags, reserved;
    uint32_t checksum;
} Record;
_Static_assert(sizeof(Record) == 36 && offsetof(Record, flags) == 28 &&
               offsetof(Record, checksum) == 32, "checkpoint layout");

static uint32_t checksum(const Record *r, size_t length)
{
    const uint8_t *data = (const uint8_t *)r;
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < length; i++)
        hash = (hash ^ data[i]) * 16777619u;
    return hash;
}

static Checkpoint decode(const Record *r)
{
    unsigned flags = r->magic[3] == '1' ? r->legacyFlags : r->flags;
    return (Checkpoint){.x = r->x, .z = r->z, .ticks = r->ticks, .saves = r->saves,
                        .yaw = r->yaw, .room = r->room, .flags = flags};
}

static bool valid(const Checkpoint *p)
{
    return p->room == Infirmary && p->yaw < 4096 && p->saves &&
           !(p->flags & ~(PianoSolved | PickupFlags | LatchOpen | NotebookRead | HandOpened)) &&
           (!(p->flags & SilverHeld) || (p->flags & PianoSolved)) &&
           (!(p->flags & HandOpened) || (p->flags & ChemicalTaken)) &&
           (!(p->flags & GoldHeld) || (p->flags & HandOpened)) &&
           p->x >= 52*4096 && p->x <= 66*4096 && p->z >= 133*4096 && p->z <= 147*4096;
}

static bool validRecord(const Record *r, size_t size)
{
    if (size == offsetof(Record, checksum) && !memcmp(r->magic, "SHC1", 4)) {
        // SHC1 stored an 8-bit flag byte and its checksum at offset 28.
        uint32_t oldChecksum;
        memcpy(&oldChecksum, (const uint8_t *)r+offsetof(Record, flags), sizeof oldChecksum);
        if ((r->legacyFlags & HandOpened) || oldChecksum != checksum(r, offsetof(Record, flags)))
            return false;
    } else if (size != sizeof *r || memcmp(r->magic, "SHC2", 4) || r->legacyFlags ||
               r->reserved || r->checksum != checksum(r, offsetof(Record, checksum))) {
        return false;
    }
    Checkpoint point = decode(r);
    return valid(&point);
}

static FILE *openCopy(const char *dir, unsigned slot, unsigned copy, const char *mode)
{
    char path[64];
    int length = snprintf(path, sizeof path, "%s/note%02u%c.sav", dir, slot+1, 'a'+copy);
    if (slot >= SaveSlots || length < 0 || (size_t)length >= sizeof path) {
        errno = EINVAL;
        return NULL;
    }
    return fopen(path, mode);
}

static SlotState readCopy(const char *dir, unsigned slot, unsigned copy, Record *r)
{
    FILE *file = openCopy(dir, slot, copy, "rb");
    if (!file) return errno == ENOENT ? SlotEmpty : SlotUnavailable;
    r->checksum = 0;
    size_t size = fread(r, 1, sizeof *r, file);
    bool ok = fgetc(file) == EOF && validRecord(r, size);
    bool failed = ferror(file);
    if (fclose(file) || failed) return SlotUnavailable;
    return ok ? SlotReady : SlotDamaged;
}

// Keep the last verified generation intact until its replacement has closed
// and can be read back from the volume.
static SlotState latest(const char *dir, unsigned slot, Record *out, unsigned *copy)
{
    Record other;
    SlotState a = readCopy(dir, slot, 0, out), b = readCopy(dir, slot, 1, &other);
    if (a == SlotUnavailable || b == SlotUnavailable) return SlotUnavailable;
    if (b == SlotReady && (a != SlotReady || (int32_t)(other.generation-out->generation) > 0)) {
        *out = other;
        *copy = 1;
        return SlotReady;
    }
    *copy = a == SlotEmpty && b == SlotDamaged ? 1 : 0;
    if (a == SlotReady) return SlotReady;
    return a == SlotEmpty && b == SlotEmpty ? SlotEmpty : SlotDamaged;
}

SlotState checkpointRead(const char *dir, unsigned slot, Checkpoint *out)
{
    Record r;
    unsigned copy;
    SlotState state = latest(dir, slot, &r, &copy);
    if (state == SlotReady) *out = decode(&r);
    return state;
}

bool checkpointWrite(const char *dir, unsigned slot, const Checkpoint *point)
{
    Record old;
    unsigned copy;
    SlotState state = latest(dir, slot, &old, &copy);
    if (state == SlotUnavailable) return false;
    if (!valid(point)) return false;
    Record r = {.magic = {'S','H','C','2'}, .generation = state == SlotReady ? old.generation+1 : 1,
                .x = point->x, .z = point->z, .ticks = point->ticks, .saves = point->saves,
                .yaw = point->yaw, .room = point->room, .flags = point->flags};
    r.checksum = checksum(&r, offsetof(Record, checksum));
    unsigned target = state == SlotEmpty ? 0 : 1-copy;
    FILE *file = openCopy(dir, slot, target, "wb");
    if (!file) return false;
    bool written = fwrite(&r, 1, sizeof r, file) == sizeof r;
    if (fclose(file) || !written) return false;
    return readCopy(dir, slot, target, &old) == SlotReady && !memcmp(&old, &r, sizeof r);
}
