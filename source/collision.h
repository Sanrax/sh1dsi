#ifndef SH1_COLLISION_H
#define SH1_COLLISION_H
#include <stdbool.h>
#include <stdio.h>

bool collisionLoad(FILE *file, bool corridor);
void collisionMove(int *worldX, int *worldZ, int dx, int dz);

#endif
