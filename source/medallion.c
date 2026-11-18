// SPDX-License-Identifier: GPL-3.0-only
// Adapted from MapEvent_PianoPuzzleInteract, Copyright (C) 2026 shdecompilations.
#include <nds.h>
#include "medallion.h"

void medallionStart(Medallion *m)
{
    *m = (Medallion){.phase = MedalZoom, .y = -10977, .z = 97198, .projection = 224};
}

bool medallionTick(Medallion *m, bool skip)
{
    if (m->phase == MedalHanging || m->phase == MedalLanded)
        return false;
    if (skip) {
        m->phase = MedalLanded;
        m->projection = 376;
        return true;
    }

    // MAP1_S01 updates 3D on alternate NTSC VBlanks. Retain the fractional
    // Q12 delta across updates so gravity and angular motion share one clock.
    if (++m->ticks & 1)
        return false;
    m->fraction += 33366u * 4096;
    int dt = m->fraction / 1000000;
    m->fraction %= 1000000;
    switch (m->phase) {
    case MedalZoom: {
        int previous = m->projection;
        m->projection = m->ticks >= 60 ? 376 : 224 + 153 * m->ticks / 60;
        if (m->ticks == 90) {
            m->ticks = 0;
            m->phase = MedalSwing;
        }
        return m->projection != previous;
    }
    case MedalSwing:
        m->angle += (dt * 227) >> 12;
        m->y = ((cosLerp(m->angle * 8) * -491) >> 12) - 10486;
        m->z = ((sinLerp(m->angle * 8) * -491) >> 12) + 97198;
        if (m->ticks == 32) {
            m->ticks = 0;
            m->phase = MedalDrop;
        }
        return true;
    case MedalDrop:
        m->velocity += dt * 98 / 10;
        m->angle += (dt * 3072) >> 12;
        m->y += (dt * m->velocity) >> 12;
        m->z += (dt * -2048) >> 12;
        if (m->y >= 0) {
            m->ticks = 0;
            m->phase = MedalImpact;
        }
        return true;
    case MedalImpact:
        if (m->ticks < 62)
            return false;
        m->phase = MedalLanded;
        return true;
    default:
        return false;
    }
}
