#ifndef SH1_LIGHTING_H
#define SH1_LIGHTING_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int16_t x, y, z;
    uint8_t u, v;
} RoomVertex;

typedef struct {
    int16_t x, y, z;
    int8_t nx, ny, nz, reserved;
} LitPoint;

typedef struct {
    uint16_t point;
    uint8_t u, v;
} LitCorner;

typedef struct {
    int x, y, z;       // Room-local PS1 Q8 coordinates.
    int dx, dy, dz;    // Q12 beam direction.
} Flashlight;

void lightingLoadRoom(bool newMap);
bool lightingTick(bool toggle);
void lightingEnableFlashlight(void);
void lightingScreen(bool enabled, int border);
unsigned lightingFog(void);
uint16_t lightingActorColor(void);
void lightingShade(const LitPoint *points, uint16_t *colors, unsigned count,
                   const Flashlight *lamp, unsigned positionShift);

#endif
