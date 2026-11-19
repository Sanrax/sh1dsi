#include "checkpoint.h"
#include "room.h"
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    char magic[4];
    uint32_t generation;
    Checkpoint point;
    uint32_t checksum;
} Record;
_Static_assert(sizeof(Record) == 32 && offsetof(Record, checksum) == 28, "checkpoint layout");

static uint32_t checksum(const Record *r)
{
    const uint8_t *data = (const uint8_t *)r;
    uint32_t hash = 2166136261u;
    for (unsigned i = 0; i < offsetof(Record, checksum); i++)
        hash = (hash ^ data[i]) * 16777619u;
    return hash;
}

static bool valid(const Record *r)
{
    const Checkpoint *p = &r->point;
    return !memcmp(r->magic, "SHC1", 4) && r->checksum == checksum(r) &&
           p->room == Infirmary && p->yaw < 4096 && p->saves &&
           !(p->flags & ~(PianoSolved | PickupFlags | LatchOpen | NotebookRead)) &&
           (!(p->flags & SilverHeld) || (p->flags & PianoSolved)) &&
           p->x >= 52*4096 && p->x <= 66*4096 && p->z >= 133*4096 && p->z <= 147*4096;
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
    bool ok = fread(r, 1, sizeof *r, file) == sizeof *r && fgetc(file) == EOF && valid(r);
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
    if (state == SlotReady) *out = r.point;
    return state;
}

bool checkpointWrite(const char *dir, unsigned slot, const Checkpoint *point)
{
    Record old;
    unsigned copy;
    SlotState state = latest(dir, slot, &old, &copy);
    if (state == SlotUnavailable) return false;
    Record r = {.magic = {'S','H','C','1'}, .generation = state == SlotReady ? old.generation+1 : 1,
                .point = *point};
    r.checksum = checksum(&r);
    if (!valid(&r)) return false;
    unsigned target = state == SlotEmpty ? 0 : 1-copy;
    FILE *file = openCopy(dir, slot, target, "wb");
    if (!file) return false;
    bool written = fwrite(&r, 1, sizeof r, file) == sizeof r;
    if (fclose(file) || !written) return false;
    return readCopy(dir, slot, target, &old) == SlotReady && !memcmp(&old, &r, sizeof r);
}
