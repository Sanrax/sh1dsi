#ifndef SH1_INSPECTION_H
#define SH1_INSPECTION_H

typedef enum { NoTowerPoint, TowerDoor, GoldenSun, SilverMoon } TowerPoint;

// Returns the final room fade length in VBlanks, or zero on an asset error.
unsigned towerInspect(const char *dir, TowerPoint point);
unsigned handInspect(const char *dir);

#endif
