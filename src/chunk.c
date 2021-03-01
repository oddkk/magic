#include "chunk.h"

size_t
chunkCoordToIndex(v3i coord)
{
	return coord.z * CHUNK_LAYER_NUM_TILES
		+  coord.y * CHUNK_WIDTH
		+  coord.x;
}

Tile *
chunkTile(struct mgc_chunk *cnk, v3i localCoord)
{
	return &cnk->tiles[chunkCoordToIndex(localCoord)];
}
