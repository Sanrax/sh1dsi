#ifndef SH1_MEDALLION_H
#define SH1_MEDALLION_H
#include <stdbool.h>

typedef enum {
    MedalHanging, MedalZoom, MedalSwing, MedalDrop, MedalImpact, MedalLanded
} MedalPhase;

typedef struct {
    MedalPhase phase;
    unsigned ticks, fraction;
    int y, z, angle, velocity, projection;
} Medallion;

void medallionStart(Medallion *m);
bool medallionTick(Medallion *m, bool skip);

#endif
