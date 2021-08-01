#ifndef MAGIC_COLOR_H
#define MAGIC_COLOR_H

#include "intdef.h"

typedef struct {
	uint8_t r, g, b;
} Color;

#define COLOR_HEX(hex) ((Color){\
		.r = ((hex) >> 16) & 0xff, \
		.g = ((hex) >>  8) & 0xff, \
		.b = ((hex) >>  0) & 0xff, \
	})

#define NOCAST_COLOR_HEX(hex) {\
		.r = ((hex) >> 16) & 0xff, \
		.g = ((hex) >>  8) & 0xff, \
		.b = ((hex) >>  0) & 0xff, \
	}

#endif
