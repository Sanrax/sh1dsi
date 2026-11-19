#ifndef SH1_CHECKPOINT_H
#define SH1_CHECKPOINT_H
#include <stdbool.h>
#include <stdint.h>

enum { SaveSlots = 11, PianoSolved = 1, SilverHeld = 2, LatchOpen = 4, NotebookRead = 8 };
enum { DrinkHeld = 16, KitHeld = 32, PickupFlags = SilverHeld | DrinkHeld | KitHeld };
typedef struct {
    int32_t x, z;
    uint32_t ticks, saves;
    uint16_t yaw;
    uint8_t room, flags;
} Checkpoint;

typedef enum { SlotEmpty, SlotReady, SlotDamaged, SlotUnavailable } SlotState;
SlotState checkpointRead(const char *dir, unsigned slot, Checkpoint *out);
bool checkpointWrite(const char *dir, unsigned slot, const Checkpoint *point);
#endif
