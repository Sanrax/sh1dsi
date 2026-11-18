// SPDX-License-Identifier: GPL-3.0-only
// Adapted from PianoPuzzle_Control and ControllerData_AnalogToDigital.
// Copyright (C) 2026 shdecompilations
#include "piano.h"

static int clamp(int n, int low, int high)
{
    if (n < low) return low;
    if (n > high) return high;
    return n;
}

static unsigned keyAt(const PianoKeyboard *kb, int x, int y)
{
    if (y >= 34) {
        static const uint8_t white[] = {0, 2, 4, 5, 7, 9, 11};
        return white[(x + 89) / 25];
    }
    unsigned key = 0;
    while (key < 11 && x >= kb->thresholds[key])
        key++;
    return key;
}

void pianoUpdate(Piano *p, const PianoKeyboard *kb, int dx, int dy,
                 bool enter, bool cancel, unsigned stepUs)
{
    if (!p->checking) {
        if (enter && !p->solved)
            *p = (Piano){.checking = true};
        return;
    }
    if (p->waitUs) {
        p->waitUs -= p->waitUs > stepUs ? stepUs : p->waitUs;
        return;
    }
    if (p->matched == 5 || cancel) {
        p->solved = p->matched == 5;
        p->checking = false;
        return;
    }
    const int q = 4096;
    p->x = clamp(p->x + dx * (45 * 4 * q / 75), -89*q, 85*q);
    p->y = clamp(p->y + dy * (45 * 4 * q / 75), -71*q, 84*q);
    if (!enter)
        return;

    static const int snap[] = {-77,-64,-52,-39,-27,-2,8,23,36,48,64,73};
    unsigned key = keyAt(kb, p->x >> 12, p->y >> 12);
    p->x = snap[key] * q;
    bool silent = key == 1 || key == 2 || key == 7 || key == 9 || key == 10;
    p->waitUs = silent ? 500000 : 300000;
    if (!silent)
        p->matched = 0;
    else if (key == kb->sequence[0])
        p->matched = 1;
    else if (key == kb->sequence[p->matched])
        p->matched++;
    else
        p->matched = 0;
}
