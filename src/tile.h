#ifndef MAGIC_TILE_H
#define MAGIC_TILE_H

#include "types.h"
#include "material.h"

struct mgc_tile {
	u16 material;
	// bit 0-14: material data
	// bit 15: clock flag
	u16 data;
};

#ifdef __GNUC__
_Static_assert(sizeof(struct mgc_tile) == 4, "struct mgc_tile must be 32-bit");
#endif

static inline int
mgc_tile_clock(struct mgc_tile t) { return !!(t.data & 0x8000); }
static inline u16
mgc_tile_data(struct mgc_tile t) { return t.data & 0x7fff; }

#endif
