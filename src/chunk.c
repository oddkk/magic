#include "chunk.h"
#include "utils.h"

struct mgc_chunk_ref
mgc_chunk_make_ref(struct mgc_chunk *chunk)
{
	struct mgc_chunk_ref result = {0};
	result.location = chunk->location;
	result.tiles = chunk->tiles;
	return result;
}

size_t
chunkCoordToIndex(v3i coord)
{
	return mgc_chunk_coord_to_index(coord);
}

size_t
mgc_chunk_coord_to_index(v3i coord)
{
	return coord.z * CHUNK_LAYER_NUM_TILES
		+  coord.y * CHUNK_WIDTH
		+  coord.x;
}

struct mgc_tile *
chunkTile(struct mgc_chunk *cnk, v3i localCoord)
{
	return &cnk->tiles[chunkCoordToIndex(localCoord)];
}

v3i
mgc_chunk_index_to_coord(size_t i)
{
	v3i result = {0};
	assert(i < CHUNK_NUM_TILES);

	result.x = i % CHUNK_WIDTH;
	result.y = (i / CHUNK_WIDTH) % CHUNK_WIDTH;
	result.z = (i / CHUNK_LAYER_NUM_TILES);

	return result;
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

v3i
mgc_coord_tile_to_chunk(v3i p)
{
	v3i result = {0};

	result.x = p.x / CHUNK_WIDTH;
	result.y = p.y / CHUNK_WIDTH;
	result.z = p.z / CHUNK_HEIGHT;

	if (p.x < 0) result.x -= 1;
	if (p.y < 0) result.y -= 1;
	if (p.z < 0) result.z -= 1;

	return result;
}

struct mgc_aabbi
mgc_coord_bounds_tile_to_chunk(struct mgc_aabbi b)
{
	struct mgc_aabbi result = {0};

	result.min = mgc_coord_tile_to_chunk(b.min);
	result.max = mgc_coord_tile_to_chunk(
		V3i(b.max.x-1, b.max.y-1, b.max.z-1)
	);

	result.max.x += 1;
	result.max.y += 1;
	result.max.z += 1;

	return result;
}

v3i
mgc_coord_chunk_to_tile(v3i p)
{
	return mgc_chunk_coord_to_world(p);
}

v3i
mgc_coord_tile_to_chunk_local(v3i p)
{
	v3i result = {0};

	result.x = p.x % CHUNK_WIDTH;
	result.y = p.y % CHUNK_WIDTH;
	result.z = p.z % CHUNK_HEIGHT;

	if (result.x < 0) result.x += CHUNK_WIDTH;
	if (result.y < 0) result.y += CHUNK_WIDTH;
	if (result.z < 0) result.z += CHUNK_HEIGHT;

	return result;
}
