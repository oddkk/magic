#include "hexGrid.h"

float hexStrideX;
float hexStrideY;
float hexStaggerX;

void
hexGridInitialize()
{
	hexStrideX = cos(30.0 * M_PI / 180.0);
	hexStrideY = sin(30.0 * M_PI / 180.0) * hexR + hexR;
	hexStaggerX = cos(30.0 * M_PI / 180.0) * hexR;
}

v2
gridDrawCoord(v2i p)
{
	v2 res;

	res.x = p.x * hexStrideX + p.y * hexStaggerX;
	res.y = p.y * hexStrideY;

	return res;
}
