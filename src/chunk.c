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

v3i
mgc_chunk_coord_to_world(v3i coord)
{
	v3i result = {0};
	result.x = coord.x * CHUNK_WIDTH;
	result.y = coord.y * CHUNK_WIDTH;
	result.z = coord.z * CHUNK_HEIGHT;
	return result;
}
