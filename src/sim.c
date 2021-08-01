#include "sim.h"
#include "utils.h"

struct mgc_sim_chunk {
	struct mgc_chunk *chunk;
	struct mgc_chunk *neighbours[10];
	struct mgc_chunk *above, *below;
};

static v3i neighbourhood[10] = {
	{.x=-1, .y=-1, .z= 0}, {.x= 0, .y=-1, .z= 0}, {.x= 1, .y=-1, .z= 0},
	{.x=-1, .y= 0, .z= 0},                        {.x= 1, .y= 0, .z= 0},
	{.x=-1, .y= 1, .z= 0}, {.x= 0, .y= 1, .z= 0}, {.x= 1, .y= 1, .z= 0},

	{.x= 0, .y= 0, .z= 1},
	{.x= 0, .y= 0, .z=-1},
};

#define NUM_SIM_CHUNKS_WIDTH (((MGC_SIM_RADIUS*2+1 + CHUNK_WIDTH-1)/CHUNK_WIDTH)+1)
#define NUM_SIM_CHUNKS_HEIGHT (((MGC_SIM_RADIUS*2+1 + CHUNK_HEIGHT-1)/CHUNK_HEIGHT)+1)
#define NUM_SIM_CHUNKS (NUM_SIM_CHUNKS_WIDTH*NUM_SIM_CHUNKS_WIDTH*NUM_SIM_CHUNKS_HEIGHT)

struct mgc_tile_neighbourhood {
	struct mgc_tile c, nw, ne, w, e, sw, se, above, below;
};

struct mgc_tile
mgc_sim_tile(struct mgc_tile_neighbourhood tiles)
{
	return tiles.c;
}

void
mgc_sim_update_tiles(struct mgc_sim_chunk *chunk, size_t start_z, size_t num_layers)
{
	size_t start_i = start_z*CHUNK_LAYER_NUM_TILES;
	size_t end_i = (start_z+num_layers)*CHUNK_LAYER_NUM_TILES;
	assert(end_i <= CHUNK_NUM_TILES);

	// Unpack the neighbourhood
	struct mgc_chunk *center = chunk->chunk;
	struct mgc_chunk *north_west = chunk->neighbours[0];
	struct mgc_chunk *north      = chunk->neighbours[1];
	struct mgc_chunk *north_east = chunk->neighbours[2];
	struct mgc_chunk *west       = chunk->neighbours[3];
	struct mgc_chunk *east       = chunk->neighbours[4];
	struct mgc_chunk *south_west = chunk->neighbours[5];
	struct mgc_chunk *south      = chunk->neighbours[6];
	struct mgc_chunk *south_east = chunk->neighbours[7];
	struct mgc_chunk *above      = chunk->neighbours[8];
	struct mgc_chunk *below      = chunk->neighbours[9];

	for (size_t i = start_i; i < end_i; i++) {
		v3i coord = mgc_chunk_index_to_coord(i);

		struct mgc_tile_neighbourhood tiles = {0};
		tiles.c = center->tiles[i];

		if (coord.z == CHUNK_HEIGHT-1) {
			if (above) {
				tiles.above = above->tiles[i%CHUNK_LAYER_NUM_TILES];
			}
		} else {
			assert(i < CHUNK_NUM_TILES - CHUNK_LAYER_NUM_TILES);
			tiles.above = center->tiles[i+CHUNK_LAYER_NUM_TILES];
		}

		if (coord.z == 0) {
			if (below) {
				size_t new_i = (CHUNK_LAYER_NUM_TILES*(CHUNK_HEIGHT-1))+(i%CHUNK_LAYER_NUM_TILES);
				tiles.below = below->tiles[new_i];
			}
		} else {
			assert(i >= CHUNK_LAYER_NUM_TILES);
			tiles.below = center->tiles[i-CHUNK_LAYER_NUM_TILES];
		}

		center->tiles[i] = mgc_sim_tile(tiles);
	}
}

void
mgc_sim_tick(v3i sim_center, struct mgc_chunk_cache *cache, struct mgc_registry *reg)
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

	struct mgc_sim_chunk sim_chunks[NUM_SIM_CHUNKS] = {0};
	size_t sim_chunks_head = 0;

	// TODO: Using mgc_chunk_cache_find is quite expensive. Optimize.
	for (int z = sim_chunk_bounds.min.z; z < sim_chunk_bounds.max.z; z++) {
		for (int y = sim_chunk_bounds.min.y; y < sim_chunk_bounds.max.y; y++) {
			for (int x = sim_chunk_bounds.min.x; x < sim_chunk_bounds.max.x; x++) {
				assert(sim_chunks_head < NUM_SIM_CHUNKS);
				v3i chunk_coord = V3i(x, y, z);
				ssize_t chunk_i = mgc_chunk_cache_find(cache, chunk_coord);

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

				sim_chunks[sim_chunks_head].chunk = chunk_entry->chunk;

				for (size_t neighbour_i = 0; neighbour_i < sizeof(neighbourhood)/sizeof(neighbourhood[0]); neighbour_i++) {
					v3i offset = neighbourhood[neighbour_i];
					ssize_t chunk_i = mgc_chunk_cache_find(cache, v3i_add(chunk_coord, offset));

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

					sim_chunks[sim_chunks_head].neighbours[neighbour_i] = chunk_entry->chunk;
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
