#ifndef MAGIC_CHUNK_H
#define MAGIC_CHUNK_H

#include "material.h"
#include "tile.h"
#include "math.h"
#include "config.h"

/*
// Tiles are hexagons, while chunks are diamond-shaped to fit together in the
// same manner as the tiles.
//
//     x         y
//    x x       y y
//   x x x     y y y
//  x x x x   y y y y
// x x x x x y y y y y
//  x x x x z y y y y w
//   x x x z z y y y w w
//    x x z z z y y w w w
//     x z z z z y w w w w
//      z z z z z w w w w w
//       z z z z   w w w w
//        z z z     w w w
//         z z       w w
//          z         w
*/

_Static_assert(CHUNK_LAYER_NUM_TILES % (sizeof(u64)*8) == 0, "Layer size must be a multiple of 64.");
_Static_assert((sizeof(u64)*8) % CHUNK_WIDTH == 0, "Layer width must be a multiple of 64.");

struct mgc_chunk {
	// The chunk's location in world coordinates.
	v3i location;

	// column-major
	struct mgc_tile tiles[CHUNK_NUM_TILES];
};

size_t
chunkCoordToIndex(v3i coord);

v3i
mgc_chunk_index_to_coord(size_t i);

v3i
mgc_chunk_coord_to_world(v3i);

v3i
mgc_coord_tile_to_chunk(v3i p);

struct mgc_aabbi
mgc_coord_bounds_tile_to_chunk(struct mgc_aabbi b);

v3i
mgc_coord_chunk_to_tile(v3i p);

v3i
mgc_coord_tile_to_chunk_local(v3i p);

struct mgc_tile *
chunkTile(struct mgc_chunk *, v3i localCoord);

#endif
