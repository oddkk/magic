#include "sim.h"
#include "utils.h"
#include "material.h"

#define NEIGHBOURHOOD_WIDTH 3
#define NEIGHBOURHOOD_SIZE (NEIGHBOURHOOD_WIDTH*NEIGHBOURHOOD_WIDTH*NEIGHBOURHOOD_WIDTH)

static v3i neighbourhood[NEIGHBOURHOOD_SIZE] = {
	{.x=-1, .y=-1, .z=-1}, {.x= 0, .y=-1, .z=-1}, {.x= 1, .y=-1, .z=-1},
	{.x=-1, .y= 0, .z=-1}, {.x= 0, .y= 0, .z=-1}, {.x= 1, .y= 0, .z=-1},
	{.x=-1, .y= 1, .z=-1}, {.x= 0, .y= 1, .z=-1}, {.x= 1, .y= 1, .z=-1},

	{.x=-1, .y=-1, .z= 0}, {.x= 0, .y=-1, .z= 0}, {.x= 1, .y=-1, .z= 0},
	{.x=-1, .y= 0, .z= 0}, {.x= 0, .y= 0, .z= 0}, {.x= 1, .y= 0, .z= 0},
	{.x=-1, .y= 1, .z= 0}, {.x= 0, .y= 1, .z= 0}, {.x= 1, .y= 1, .z= 0},

	{.x=-1, .y=-1, .z= 1}, {.x= 0, .y=-1, .z= 1}, {.x= 1, .y=-1, .z= 1},
	{.x=-1, .y= 0, .z= 1}, {.x= 0, .y= 0, .z= 1}, {.x= 1, .y= 0, .z= 1},
	{.x=-1, .y= 1, .z= 1}, {.x= 0, .y= 1, .z= 1}, {.x= 1, .y= 1, .z= 1},
};

struct mgc_sim_context {
	struct mgc_sim_chunk *chunks;
	v3i coord;
};

// This routine requires the offset to be constrained to +-(32,32,32).
struct mgc_tile *
mgc_sim_get_tile(struct mgc_sim_context ctx, v3i offset)
{
#if 1
	int x = ctx.coord.x + offset.x;
	int y = ctx.coord.y + offset.y;
	int z = ctx.coord.z + offset.z;

#if 1
	size_t chunk_i = 0 // (NEIGHBOURHOOD_SIZE/2)
		+ ((x + CHUNK_WIDTH)  >> LOG_CHUNK_WIDTH)
		+ ((y + CHUNK_WIDTH)  >> LOG_CHUNK_WIDTH)  * 3
		+ ((z + CHUNK_HEIGHT) >> LOG_CHUNK_HEIGHT) * 9;
#else
	size_t chunk_i = (NEIGHBOURHOOD_SIZE/2)
		+ (x / CHUNK_WIDTH)
		+ (y / CHUNK_WIDTH)  * 3
		+ (z / CHUNK_HEIGHT) * 9;
#endif

#if 1
	x = x & ((1<<(LOG_CHUNK_WIDTH)) -1);
	y = y & ((1<<(LOG_CHUNK_WIDTH)) -1);
	z = z & ((1<<(LOG_CHUNK_HEIGHT))-1);
#else
	x = (x + CHUNK_WIDTH)  % CHUNK_WIDTH;
	y = (y + CHUNK_WIDTH)  % CHUNK_WIDTH;
	z = (z + CHUNK_HEIGHT) % CHUNK_HEIGHT;
#endif

	size_t tile_i = x | y << LOG_CHUNK_WIDTH | z << (LOG_CHUNK_WIDTH*2);

	return &ctx.chunks->neighbours[chunk_i].tiles[tile_i];
#else
	v3i coord = v3i_add(ctx.coord, offset);
	// Chunk id 13 is the center chunk.
	int chunk_id = 13;

	if (coord.x <  0)           chunk_id -= 1;
	if (coord.x >= CHUNK_WIDTH) chunk_id += 1;

	if (coord.y <  0)           chunk_id -= CHUNK_WIDTH;
	if (coord.y >= CHUNK_WIDTH) chunk_id += CHUNK_WIDTH;

	if (coord.z <  0)           chunk_id -= CHUNK_LAYER_NUM_TILES;
	if (coord.z >= CHUNK_WIDTH) chunk_id += CHUNK_LAYER_NUM_TILES;

	coord.x = (coord.x + CHUNK_WIDTH)  % CHUNK_WIDTH;
	coord.y = (coord.y + CHUNK_WIDTH)  % CHUNK_WIDTH;
	coord.z = (coord.z + CHUNK_HEIGHT) % CHUNK_HEIGHT;
#if 0
	assert(
		coord.x >= 0 && coord.x < CHUNK_WIDTH &&
		coord.y >= 0 && coord.y < CHUNK_WIDTH &&
		coord.z >= 0 && coord.z < CHUNK_HEIGHT
	);
	assert(
		chunk_id >= 0 &&
		chunk_id < (sizeof(ctx.chunks->neighbours) / sizeof(ctx.chunks->neighbours[0]))
	);
#endif

	size_t tile_i = mgc_chunk_coord_to_index(coord);

	return &ctx.chunks->neighbours[chunk_id].tiles[tile_i];
#endif
}

static inline void
mgc_tile_swap(struct mgc_tile *t1, struct mgc_tile *t2)
{
	struct mgc_tile tmp = *t1;
	*t1 = *t2;
	*t2 = tmp;
}

void
mgc_sim_tile(struct mgc_sim_context ctx)
{
	struct mgc_tile *tile = mgc_sim_get_tile(ctx, V3i(0, 0, 0));
	struct mgc_tile *below = mgc_sim_get_tile(ctx, V3i(0, 0, -1));

	if (tile->material == MAT_SAND) {
		if (below->material == MAT_AIR) {
			mgc_tile_swap(tile, below);
			return;
		}
	}

	if (tile->material == MAT_WATER) {
		if (below->material == MAT_AIR) {
			mgc_tile_swap(tile, below);
			return;
		}

		for (int y = -1; y <= 1; y++) {
			for (int x = -1; x <= 1; x++) {
				if (y == x) continue;
				struct mgc_tile *other = mgc_sim_get_tile(ctx, V3i(x, y, 0));

				if (other->material == MAT_AIR) {
					*other = *tile;
				}
			}
		}
	}
}

void
mgc_sim_update_tiles(struct mgc_sim_chunk *chunk, size_t start_z, size_t num_layers)
{
	size_t start_i = start_z*CHUNK_LAYER_NUM_TILES;
	size_t end_i = (start_z+num_layers)*CHUNK_LAYER_NUM_TILES;
	assert(end_i <= CHUNK_NUM_TILES);

	struct mgc_sim_context sim_ctx = {0};
	sim_ctx.chunks = chunk;

	for (size_t i = start_i; i < end_i; i++) {
		sim_ctx.coord = mgc_chunk_index_to_coord(i);
		mgc_sim_tile(sim_ctx);
	}
}

void
mgc_sim_tick(
		struct mgc_sim_buffer *buffer,
		v3i sim_center,
		struct mgc_chunk_cache *cache,
		struct mgc_registry *reg)
{
	struct mgc_aabbi sim_bounds, skirt_bounds;
	sim_bounds = mgc_aabbi_from_radius(sim_center, MGC_SIM_RADIUS);
	skirt_bounds = mgc_aabbi_from_radius(sim_center, MGC_SIM_RADIUS+MGC_SIM_SKIRT_RADIUS);

	struct mgc_aabbi load_chunk_bounds, sim_chunk_bounds;
	load_chunk_bounds = mgc_coord_bounds_tile_to_chunk(skirt_bounds);
	sim_chunk_bounds = mgc_coord_bounds_tile_to_chunk(sim_bounds);

	for (int z = load_chunk_bounds.min.z; z < load_chunk_bounds.max.z; z++) {
		for (int y = load_chunk_bounds.min.y; y < load_chunk_bounds.max.y; y++) {
			for (int x = load_chunk_bounds.min.x; x < load_chunk_bounds.max.x; x++) {
				mgc_chunk_cache_request(cache, V3i(x, y, z));
			}
		}
	}

	mgc_chunk_cache_tick(cache);

	memset(buffer, 0, sizeof(struct mgc_sim_buffer));
	struct mgc_sim_chunk *sim_chunks = buffer->sim_chunks;
	size_t sim_chunks_head = 0;


	// TODO: Using mgc_chunk_cache_find is quite expensive. Optimize.
	for (int z = sim_chunk_bounds.min.z; z < sim_chunk_bounds.max.z; z++) {
		for (int y = sim_chunk_bounds.min.y; y < sim_chunk_bounds.max.y; y++) {
			for (int x = sim_chunk_bounds.min.x; x < sim_chunk_bounds.max.x; x++) {
				assert(sim_chunks_head < NUM_SIM_CHUNKS);
				v3i chunk_coord = V3i(x, y, z);

				for (size_t neighbour_i = 0; neighbour_i < sizeof(neighbourhood)/sizeof(neighbourhood[0]); neighbour_i++) {
					v3i offset = neighbourhood[neighbour_i];
					isize chunk_i = mgc_chunk_cache_find(cache, v3i_add(chunk_coord, offset));

					if (chunk_i < 0) {
						continue;
					}

					struct mgc_chunk_cache_entry *chunk_entry;
					chunk_entry = &cache->entries[chunk_i];

					if (chunk_entry->state != MGC_CHUNK_CACHE_LOADED &&
						chunk_entry->state != MGC_CHUNK_CACHE_MESHED && 
						chunk_entry->state != MGC_CHUNK_CACHE_DIRTY) {
						continue;
					}

					assert(chunk_entry->chunk);

					sim_chunks[sim_chunks_head].neighbours[neighbour_i] =
						mgc_chunk_make_ref(chunk_entry->chunk);
				}

				sim_chunks_head += 1;
			}
		}
	}

	// TODO: Paralellize by splitting the chunk into layers. We could update
	// multiple chunks at the same time by only updating alternating layers
	// that are not adjacent to any other layer that is currently being
	// updated.

	// Update said chunks
	for (size_t chunk_i = 0; chunk_i < sim_chunks_head; chunk_i++) {
		struct mgc_sim_chunk *chunk;
		chunk = &sim_chunks[chunk_i];

		for (size_t batch = 0; batch < CHUNK_HEIGHT/SIM_CHUNK_BATCH_SIZE_LAYERS; batch++) {
			mgc_sim_update_tiles(
				chunk,
				batch*SIM_CHUNK_BATCH_SIZE_LAYERS,
				SIM_CHUNK_BATCH_SIZE_LAYERS
			);
		}
	}
}
