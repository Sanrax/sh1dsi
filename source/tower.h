#ifndef SH1_TOWER_H
#define SH1_TOWER_H

typedef enum { NoTowerPoint, TowerDoor, GoldenSun, SilverMoon } TowerPoint;

// Returns the final room fade length in VBlanks, or zero on an asset error.
unsigned towerInspect(const char *dir, TowerPoint point);

#endif
