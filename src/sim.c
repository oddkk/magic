#include "sim.h"
#include "utils.h"
#include "material.h"

#include "profile.h"

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

#if (64 % RENDER_CHUNKS_PER_CHUNK != 0)
#error "The number of render chunks must be a factor of 64"
#endif

#define DIVIDE_CEILING(a, b) (((a)+(b)-1) / (b))
#define SIM_CTX_CHANGED_MASK_SIZE (NEIGHBOURHOOD_SIZE*RENDER_CHUNKS_PER_CHUNK)
#define SIM_CTX_CHANGED_MASK_BIT_PER_UNIT (sizeof(u64)*8)
#define SIM_CTX_CHANGED_SIZE DIVIDE_CEILING(SIM_CTX_CHANGED_MASK_SIZE, SIM_CTX_CHANGED_MASK_BIT_PER_UNIT)
struct mgc_sim_context {
	struct mgc_sim_chunk *chunks;
	v3i coord;
	bool clock;
	chunk_dirty_mask_t *changed;
};

struct mgc_tile_ref {
	struct mgc_tile *tile;
	u64 chunk_mask;
	u32 neighbour_idx;
	u32 render_chunk_idx;
};

// This routine requires the offset to be constrained to +-(32,32,32).
struct mgc_tile_ref
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

	size_t render_chunk_i =
		  (x / RENDER_CHUNK_WIDTH)
		+ (y / RENDER_CHUNK_WIDTH) * RENDER_CHUNKS_PER_CHUNK_WIDTH
		+ (z / RENDER_CHUNK_HEIGHT) * RENDER_CHUNKS_PER_CHUNK_LAYER;

	size_t tile_i = x | y << LOG_CHUNK_WIDTH | z << (LOG_CHUNK_WIDTH*2);

	struct mgc_tile_ref tile_ref = {0};
	tile_ref.neighbour_idx = chunk_i;
	tile_ref.render_chunk_idx = render_chunk_i;
	tile_ref.tile = &ctx.chunks->neighbours[chunk_i].tiles[tile_i];

	return tile_ref;
}

#define INLINE_TILE_INSTRS 0

#define _TM_TOUCHED(m) ((m >> 1) ^ (ctx.clock ? 0 : 0x4000))
#define TILE_UPDATED(ref) \
	(((ref).tile->material) & (_TM_TOUCHED((ref).tile->material) & 0x4000))
#define TILE(mat, data) ((struct mgc_tile){mat, data})
#define TILE_SET(dst, tile_data) do { \
	if (!TILE_UPDATED(dst)) { \
		(dst).tile->material = (ctx.clock ? 0xc000 : 0x4000) | ((tile_data).material & 0x3fff); \
		(dst).tile->data = ((tile_data).data); \
		ctx.changed[dst.neighbour_idx] |= \
			(1UL << dst.render_chunk_idx); \
	} \
} while(0);
#define TILE_SWAP(t1, t2) do { \
	if (!TILE_UPDATED(t1) && !TILE_UPDATED(t2)) { \
		struct mgc_tile tmp = *(t2).tile; \
		TILE_SET((t2), *(t1).tile); \
		TILE_SET((t1), tmp); \
	} \
} while(0);
#define MAT(ref) (mgc_tile_material(*(ref).tile))
#define DATA(ref) ((ref).tile->data)

#if !INLINE_TILE_INSTRS

#define ALWAYS_INLINE __attribute__ ((always_inline))

static bool ALWAYS_INLINE
mgc_sim_impl_tm_touched(struct mgc_sim_context ctx, u16 m)
{
	return _TM_TOUCHED(m);
}
#undef _TM_TOUCHED
#define _TM_TOUCHED(...) mgc_sim_impl_tm_touched(ctx, __VA_ARGS__)

static bool ALWAYS_INLINE
mgc_sim_impl_tile_updated(struct mgc_sim_context ctx, struct mgc_tile_ref tile)
{
	return TILE_UPDATED(tile);
}
#undef TILE_UPDATED
#define TILE_UPDATED(...) mgc_sim_impl_tile_updated(ctx, __VA_ARGS__)

static struct mgc_tile ALWAYS_INLINE
mgc_sim_impl_tile(u16 mat, u16 data)
{
	return TILE(mat, data);
}
#undef TILE
#define TILE mgc_sim_impl_tile

static void ALWAYS_INLINE
mgc_sim_impl_tile_set(struct mgc_sim_context ctx, struct mgc_tile_ref dst, struct mgc_tile tile_data)
{
	TILE_SET(dst, tile_data);
}
#undef TILE_SET
#define TILE_SET(...) mgc_sim_impl_tile_set(ctx, __VA_ARGS__)

static void ALWAYS_INLINE
mgc_sim_impl_tile_swap(struct mgc_sim_context ctx, struct mgc_tile_ref t1, struct mgc_tile_ref t2)
{
	TILE_SWAP(t1, t2);
}
#undef TILE_SWAP
#define TILE_SWAP(...) mgc_sim_impl_tile_swap(ctx, __VA_ARGS__)

static int ALWAYS_INLINE
mgc_sim_impl_mat(struct mgc_tile_ref tile)
{
	return MAT(tile);
}
#undef MAT
#define MAT mgc_sim_impl_mat

/*
static u16
mgc_sim_impl_data(struct mgc_tile_ref tile)
{
	return DATA(tile);
}
#undef DATA
#define DATA mgc_sim_impl_data
*/

#undef ALWAYS_INLINE

#endif

void
mgc_sim_tile(struct mgc_sim_context ctx, struct mgc_tile_ref tile)
{

	struct mgc_tile_ref below = mgc_sim_get_tile(ctx, V3i(0, 0, -1));
	struct mgc_tile_ref above = mgc_sim_get_tile(ctx, V3i(0, 0,  1));

	switch (MAT(tile)) {

		case MAT_SAND:
			if (MAT(below) == MAT_AIR) {
				DATA(tile) = 1;
				TILE_SWAP(tile, below);
				return;
			}

			if (DATA(tile) == 1) {
				for (int y = -1; y <= 1; y++) {
					for (int x = -1; x <= 1; x++) {
						if (y == x) continue;
						struct mgc_tile_ref other = mgc_sim_get_tile(ctx, V3i(x, y, -1));

						if (MAT(other) == MAT_AIR) {
							TILE_SWAP(other, tile);
							return;
						}
					}
				}
			}

			DATA(tile) = 0;
			break;

		case MAT_WATER:
			if (MAT(below) == MAT_AIR) {
				TILE_SWAP(tile, below);
				return;
			}

			if (MAT(below) == MAT_WATER) {
				if (MAT(above) == MAT_AIR) {
					TILE_SET(tile, TILE(MAT_AIR, 0));
				}
				return;
			}

			for (int y = -1; y <= 1; y++) {
				for (int x = -1; x <= 1; x++) {
					if (y == x) continue;
					struct mgc_tile_ref other = mgc_sim_get_tile(ctx, V3i(x, y, 0));

					if (MAT(other) == MAT_AIR) {
						TILE_SET(other, *tile.tile);
					}
				}
			}
			break;

	}
}

#undef TILE
#undef TILE_SET
#undef TILE_SWAP
#undef TILE_UPDATED
#undef _TM_TOUCHED
#undef MAT
#undef DATA

void
mgc_sim_update_tiles(struct mgc_sim_chunk *chunk, size_t start_z, size_t num_layers, bool clock)
{
	size_t start_i = start_z*CHUNK_LAYER_NUM_TILES;
	size_t end_i = (start_z+num_layers)*CHUNK_LAYER_NUM_TILES;
	assert(end_i <= CHUNK_NUM_TILES);

	struct mgc_sim_context sim_ctx = {0};
	sim_ctx.chunks = chunk;
	sim_ctx.clock = clock;

	chunk_dirty_mask_t changed[NEIGHBOURHOOD_SIZE] = {0};
	sim_ctx.changed = changed;

	for (size_t i = start_i; i < end_i;) {
		size_t strip_start = i;
		size_t strip_end = i + RENDER_CHUNK_WIDTH;
		for (; i < strip_end; i++) {
			sim_ctx.coord = mgc_chunk_index_to_coord(i);

			struct mgc_tile_ref tile = mgc_sim_get_tile(sim_ctx, V3i(0, 0, 0));
			if (!!(tile.tile->material & 0x8000) != clock) {
				tile.tile->material =
					(clock ? 0x8000 : 0x0000) |
					(tile.tile->material & 0x3fff);
				mgc_sim_tile(sim_ctx, tile);
			}
		}

		u64 rchunk_idx = strip_start / RENDER_CHUNK_WIDTH;
		u64 rchunk_x = rchunk_idx % RENDER_CHUNKS_PER_CHUNK_WIDTH;
		u64 rchunk_y = (rchunk_idx / (RENDER_CHUNKS_PER_CHUNK_WIDTH * RENDER_CHUNK_WIDTH)) % RENDER_CHUNKS_PER_CHUNK_WIDTH;
		u64 rchunk_z = (rchunk_idx / (RENDER_CHUNKS_PER_CHUNK_WIDTH * CHUNK_WIDTH * RENDER_CHUNK_WIDTH));
		u64 rchunk_i = rchunk_x +
			(rchunk_y * RENDER_CHUNKS_PER_CHUNK_WIDTH) +
			(rchunk_z * RENDER_CHUNKS_PER_CHUNK_LAYER);
		assert(rchunk_i < RENDER_CHUNKS_PER_CHUNK);
	}

	for (size_t i = 0; i < NEIGHBOURHOOD_SIZE; i++) {
		chunk->cache_entry[i]->dirty_mask |= changed[i];
	}
}

void
mgc_sim_tick(
		struct mgc_sim_buffer *buffer,
		struct mgc_chunk_cache *cache,
		struct mgc_registry *reg,
		u64 sim_tick)
{
	TracyCZone(trace, true);

	memset(buffer, 0, sizeof(struct mgc_sim_buffer));
	struct mgc_sim_chunk *sim_chunks = buffer->sim_chunks;
	size_t sim_chunks_head = 0;

	TracyCZoneN(trace_find_chunks, "find chunks to simulate", true);
	struct mgc_aabbi sim_bounds, sim_chunk_bounds;
	sim_bounds = mgc_aabbi_from_radius(cache->sim_center, MGC_SIM_RADIUS);
	sim_chunk_bounds = mgc_coord_bounds_tile_to_chunk(sim_bounds);

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


					sim_chunks[sim_chunks_head].cache_entry[neighbour_i] = chunk_entry;
					sim_chunks[sim_chunks_head].neighbours[neighbour_i] =
						mgc_chunk_make_ref(chunk_entry->chunk);
				}

				// The center chunk was not loaded, so don't try simulating it.
				if (!sim_chunks[sim_chunks_head].cache_entry[NEIGHBOURHOOD_CENTER_IDX]) {
					memset(&sim_chunks[sim_chunks_head], 0, sizeof(struct mgc_sim_chunk));
					continue;
				}

				sim_chunks_head += 1;
			}
		}
	}
	TracyCZoneEnd(trace_find_chunks);

	// TODO: Paralellize by splitting the chunk into layers. We could update
	// multiple chunks at the same time by only updating alternating layers
	// that are not adjacent to any other layer that is currently being
	// updated.

	bool clock = (sim_tick % 2) == 1;

	TracyCZoneN(trace_sim, "simulate chunks", true);
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
	TracyCZoneEnd(trace_sim);

	TracyCZoneEnd(trace);
}
