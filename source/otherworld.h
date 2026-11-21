#ifndef SH1_OTHERWORLD_H
#define SH1_OTHERWORLD_H

#include <stdbool.h>
#include <stdio.h>
#include "player.h"

bool otherworldLoadIntro(FILE *file);
void otherworldStartIntro(void);
bool otherworldIntroActive(void);
bool otherworldTickIntro(bool skip);
void otherworldDrawSubtitle(int bg);
CameraPose otherworldIntroView(void);
int otherworldIntroProjection(void);

#endif
