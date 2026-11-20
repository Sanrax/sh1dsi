#ifndef SH1_STATUE_H
#define SH1_STATUE_H
#include <stdbool.h>
#include <stdio.h>

enum { StatueRevealVBlanks = 33 };
bool statueLoad(FILE *file, unsigned textureOffset, unsigned palette);
bool statueActive(void);
bool statueCamera(void);
bool statueOpened(void);
void statueRestore(unsigned flags);
unsigned statueFlags(void);
bool statueStart(void);
bool statueTick(unsigned pressed);
unsigned statueBorder(void);
void statueDrawSmoke(void);
#endif
