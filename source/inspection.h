#ifndef SH1_INSPECTION_H
#define SH1_INSPECTION_H
#include <stdbool.h>

typedef enum { NoTowerPoint, TowerDoor, GoldenSun, SilverMoon } TowerPoint;

// Returns the final room fade length in VBlanks, or zero on an asset error.
unsigned towerInspect(const char *dir, TowerPoint point);
bool towerCanPlace(TowerPoint point, unsigned items);
unsigned towerPlace(const char *dir, TowerPoint point, void (*playSound)(bool chime));
unsigned towerFlags(void);
void towerRestore(unsigned flags);
unsigned handInspect(const char *dir);

#endif
