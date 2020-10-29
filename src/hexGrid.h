#ifndef MAGIC_HEX_GRID_H
#define MAGIC_HEX_GRID_H

#include <math.h>
#include "math.h"

static const float hexR = 0.5;
extern float hexStrideX;
extern float hexStrideY;
extern float hexStaggerX;

void
hexGridInitialize();

v2f
gridDrawCoord(v2i);

#endif
