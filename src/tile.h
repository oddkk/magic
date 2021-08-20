#ifndef MAGIC_TILE_H
#define MAGIC_TILE_H

#include "types.h"
#include "material.h"

struct mgc_tile {
	// bit 0-13: material id
	// bit 14: clock
	u16 material;
	u16 data;
};

#ifdef __GNUC__
_Static_assert(sizeof(struct mgc_tile) == 4, "struct mgc_tile must be 32-bit");
#endif

static inline int
mgc_tile_material(struct mgc_tile t) { return t.material & 0x3fff; }
static inline u16
mgc_tile_data(struct mgc_tile t) { return t.data; }

#endif
