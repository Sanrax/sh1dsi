#ifndef SH1_ROOM_H
#define SH1_ROOM_H
#include <stdbool.h>
#include <stdio.h>
#include "medallion.h"

bool roomLoad(FILE *file);
void roomDraw(const Medallion *medal);

#endif
