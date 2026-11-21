#ifndef SH1_PHONE_H
#define SH1_PHONE_H

#include "player.h"

bool phoneLoad(const char *directory);
void phoneUnload(void);
bool phoneComplete(void);
bool phoneStart(void);
bool phoneActive(void);
bool phoneTick(unsigned keys);
void phoneDrawText(int bg);
int phoneBorder(void);
int phoneProjection(void);
bool phoneView(CameraPose *view);
bool phoneDeskVisible(void);
bool phoneHolding(void);
void phoneDrawReceiver(unsigned texture, unsigned palette, unsigned fog);

enum { PhoneReveal = 45 };

#endif
