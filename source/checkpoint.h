#ifndef SH1_CHECKPOINT_H
#define SH1_CHECKPOINT_H
#include <stdbool.h>
#include <stdint.h>

enum { SaveSlots = 11, PianoSolved = 1, SilverHeld = 2, LatchOpen = 4, NotebookRead = 8 };
enum { DrinkHeld = 16, KitHeld = 32, ChemicalTaken = 64 };
enum { HandOpened = 128, GoldHeld = 256 };
enum { SunPlaced = 512, MoonPlaced = 1024, TowerFlags = SunPlaced | MoonPlaced };
enum { BoilerPowered = 2048 };
enum { LobbyKitHeld = 4096, LobbyBulletsHeld = 8192, PictureCardHeld = 16384, RubberBallHeld = 32768 };
enum { PickupFlags = SilverHeld | DrinkHeld | KitHeld | ChemicalTaken | GoldHeld |
                     LobbyKitHeld | LobbyBulletsHeld | PictureCardHeld | RubberBallHeld };
typedef struct {
    int32_t x, z;
    uint32_t ticks, saves;
    uint16_t yaw, flags;
    uint8_t room;
} Checkpoint;

typedef enum { SlotEmpty, SlotReady, SlotDamaged, SlotUnavailable } SlotState;
SlotState checkpointRead(const char *dir, unsigned slot, Checkpoint *out);
bool checkpointWrite(const char *dir, unsigned slot, const Checkpoint *point);
#endif
