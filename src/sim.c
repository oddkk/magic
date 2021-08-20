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
	bool clock;
};

// This routine requires the offset to be constrained to +-(32,32,32).
struct mgc_tile *
mgc_sim_get_tile(struct mgc_sim_context ctx, v3i offset)
{
	int x = ctx.coord.x + offset.x;
	int y = ctx.coord.y + offset.y;
	int z = ctx.coord.z + offset.z;

	size_t chunk_i =
		  ((x + CHUNK_WIDTH)  >> LOG_CHUNK_WIDTH)
		+ ((y + CHUNK_WIDTH)  >> LOG_CHUNK_WIDTH)  * 3
		+ ((z + CHUNK_HEIGHT) >> LOG_CHUNK_HEIGHT) * 9;

	x = x & ((1<<(LOG_CHUNK_WIDTH)) -1);
	y = y & ((1<<(LOG_CHUNK_WIDTH)) -1);
	z = z & ((1<<(LOG_CHUNK_HEIGHT))-1);

	size_t tile_i = x | y << LOG_CHUNK_WIDTH | z << (LOG_CHUNK_WIDTH*2);

	return &ctx.chunks->neighbours[chunk_i].tiles[tile_i];
}

void
mgc_sim_tile(struct mgc_sim_context ctx, struct mgc_tile *tile)
{
#define _TM_TOUCHED(m) ((m >> 1) ^ (ctx.clock ? 0 : 0x4000))
#define TILE_UPDATED(tile) \
	(((tile).material) & (_TM_TOUCHED((tile).material) & 0x4000))
#define TILE(mat, data) ((struct mgc_tile){mat, data})
#define TILE_SET(dst, tile_data) do { \
	if (!TILE_UPDATED(*(dst))) { \
		(dst)->material = (ctx.clock ? 0xc000 : 0x4000) | ((tile_data).material & 0x3fff); \
		(dst)->data = ((tile_data).data); \
	} \
} while(0);
#define TILE_SWAP(t1, t2) do { \
	if (!TILE_UPDATED(*(t1)) && !TILE_UPDATED(*(t2))) { \
		struct mgc_tile tmp = *(t2); \
		TILE_SET((t2), *(t1)); \
		TILE_SET((t1), tmp); \
	} \
} while(0);
#define MAT(tile) (mgc_tile_material(tile))

	struct mgc_tile *below = mgc_sim_get_tile(ctx, V3i(0, 0, -1));
	struct mgc_tile *above = mgc_sim_get_tile(ctx, V3i(0, 0,  1));

	switch (mgc_tile_material(*tile)) {
		case MAT_SAND:
			if (MAT(*below) == MAT_AIR) {
				TILE_SWAP(tile, below);
				return;
			}
			break;
		case MAT_WATER:
			if (MAT(*below) == MAT_AIR) {
				TILE_SWAP(tile, below);
				return;
			}

			if (MAT(*below) == MAT_WATER) {
				if (MAT(*above) == MAT_AIR) {
					TILE_SET(tile, TILE(MAT_AIR, 0));
				}
				return;
			}

			for (int y = -1; y <= 1; y++) {
				for (int x = -1; x <= 1; x++) {
					if (y == x) continue;
					struct mgc_tile *other = mgc_sim_get_tile(ctx, V3i(x, y, 0));

					if (MAT(*other) == MAT_AIR) {
						TILE_SET(other, *tile);
					}
				}
			}
			break;
	}

#undef TILE
#undef TILE_SET
#undef TILE_SWAP
#undef TILE_UPDATED
#undef _TM_TOUCHED
}

void
mgc_sim_update_tiles(struct mgc_sim_chunk *chunk, size_t start_z, size_t num_layers, bool clock)
{
	size_t start_i = start_z*CHUNK_LAYER_NUM_TILES;
	size_t end_i = (start_z+num_layers)*CHUNK_LAYER_NUM_TILES;
	assert(end_i <= CHUNK_NUM_TILES);

	struct mgc_sim_context sim_ctx = {0};
	sim_ctx.chunks = chunk;
	sim_ctx.clock = clock;

	for (size_t i = start_i; i < end_i; i++) {
		sim_ctx.coord = mgc_chunk_index_to_coord(i);

		struct mgc_tile *tile = mgc_sim_get_tile(sim_ctx, V3i(0, 0, 0));
		if (!!(tile->material & 0x8000) != clock) {
			tile->material =
				(clock ? 0x8000 : 0x0000) |
				(tile->material & 0x3fff);
			mgc_sim_tile(sim_ctx, tile);
		}
	}
}

void
mgc_sim_tick(
		struct mgc_sim_buffer *buffer,
		v3i sim_center,
		struct mgc_chunk_cache *cache,
		struct mgc_registry *reg,
		u64 sim_tick)
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

	bool clock = (sim_tick % 2) == 1;

	// Update said chunks
	for (size_t chunk_i = 0; chunk_i < sim_chunks_head; chunk_i++) {
		struct mgc_sim_chunk *chunk;
		chunk = &sim_chunks[chunk_i];

		for (size_t batch = 0; batch < CHUNK_HEIGHT/SIM_CHUNK_BATCH_SIZE_LAYERS; batch++) {
			mgc_sim_update_tiles(
				chunk,
				batch*SIM_CHUNK_BATCH_SIZE_LAYERS,
				SIM_CHUNK_BATCH_SIZE_LAYERS,
				clock
			);
		}
	}
}
