#ifndef MAGIC_CHUNK_H
#define MAGIC_CHUNK_H

#include "material.h"
#include "tile.h"
#include "math.h"

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

#define CHUNK_WIDTH 16
#define CHUNK_HEIGHT 1
#define CHUNK_TILES (CHUNK_WIDTH*CHUNK_WIDTH*CHUNK_HEIGHT)

typedef struct mgc_chunk_t {
	// The abstraction level of the chunk. For level 0 each tile equals one
	// block. For for level n, n > 0, each tile equals one chunk of level n-1.
	unsigned int level;

	// The chunk's location in world coordinates for its level.
	v3i location;

	Tile tiles[CHUNK_TILES];
	struct mgc_chunk_t **childChunks;
} Chunk;

#endif
