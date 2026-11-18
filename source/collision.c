// Local XZ collision for the flat SCFD00 floor. Edges and tangent vectors
// come from the IPD splits used by the original character query.
#include <nds.h>
#include "collision.h"

typedef struct {
    int16_t ax, az, bx, bz, tx, tz;
} Edge;
static Edge edges[51];
enum { Radius = 1228 }; // Harry's 0.3-metre outer cylinder, Q12.

bool collisionLoad(FILE *file)
{
    if (fread(edges, sizeof edges, 1, file) != 1) return false;
    for (unsigned i = 0; i < 51; i++) {
        const Edge *e = &edges[i];
        if (abs(e->ax) > 20000 || abs(e->az) > 20000 ||
            abs(e->bx) > 20000 || abs(e->bz) > 20000 ||
            abs(e->tx) > 4096 || abs(e->tz) > 4096 ||
            (e->ax == e->bx && e->az == e->bz))
            return false;
    }
    return true;
}

static bool pushFromEdge(const Edge *e, int *x, int *z)
{
    int fromAX = *x-e->ax, fromAZ = *z-e->az;
    int fromBX = *x-e->bx, fromBZ = *z-e->bz;
    if ((fromAX > Radius && fromBX > Radius) || (fromAX < -Radius && fromBX < -Radius) ||
        (fromAZ > Radius && fromBZ > Radius) || (fromAZ < -Radius && fromBZ < -Radius))
        return false;

    int nx, nz, distance;
    if (fromBX*e->tx + fromBZ*e->tz < 0 || fromAX*e->tx + fromAZ*e->tz > 0) {
        // Beyond the segment, use its rounded end.
        int dx = fromBX, dz = fromBZ;
        if (fromAX*e->tx + fromAZ*e->tz > 0) {
            dx = fromAX;
            dz = fromAZ;
        }
        if (abs(dx) >= Radius || abs(dz) >= Radius) return false;
        distance = sqrt32(dx*dx + dz*dz);
        if (distance >= Radius || !distance) return false;
        nx = dx*4096/distance;
        nz = dz*4096/distance;
    } else {
        nx = -e->tz;
        nz = e->tx;
        int side = (fromBX*nx + fromBZ*nz) >> 12;
        distance = abs(side);
        if (distance >= Radius) return false;
        if (side < 0) { nx = -nx; nz = -nz; }
    }
    *x += (nx*(Radius-distance)) >> 12;
    *z += (nz*(Radius-distance)) >> 12;
    return true;
}

void collisionMove(int *worldX, int *worldZ, int dx, int dz)
{
    int x = *worldX + 409600 + dx, z = *worldZ - 81920 + dz;
    // The largest walking step is under five centimetres. A second pass
    // resolves adjacent segments when sliding into a corner.
    for (unsigned pass = 0; pass < 2; pass++) {
        bool contact = false;
        for (unsigned i = 0; i < 51; i++)
            contact |= pushFromEdge(&edges[i], &x, &z);
        if (!contact) break;
    }
    *worldX = x - 409600;
    *worldZ = z + 81920;
}
