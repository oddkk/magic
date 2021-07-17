#ifndef MAGIC_HEX_GRID_H
#define MAGIC_HEX_GRID_H

#include <math.h>
#include "math.h"

// Height
static const float hexH = 0.25;

// Radius
static const float hexR = 0.25;
extern float hexStrideX;
extern float hexStrideY;
extern float hexStaggerX;

void
hexGridInitialize();

v2f
gridDrawCoord(v2i);

v3
mgc_grid_draw_coord(v3i p);

#endif
