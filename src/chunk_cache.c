#include "chunk_cache.h"
#include "config.h"
#include "utils.h"
#include "world.h"
#include "render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "profile.h"

#if MGCD_DEBUG_CHUNK_CACHE
static void
mgccc_debug_trace(v3i coord, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	FILE *out = stdout;

	fprintf(out, "[cc %i,%i,%i] ", coord.x, coord.y, coord.z);
	vfprintf(out, fmt, ap);
	fprintf(out, "\n");
	va_end(ap);

}
#else
#define mgccc_debug_trace(...)
#endif

void
mgc_chunk_cache_init(struct mgc_chunk_cache *cache, struct mgc_memory *memory, struct mgc_world *world, struct mgc_material_table *mat_table)
{
	memset(cache, 0, sizeof(struct mgc_chunk_cache));
	cache->world = world;
	cache->mat_table = mat_table;
	cache->cap_entries = MGC_CHUNK_CACHE_SIZE;
	cache->entries = calloc(cache->cap_entries, sizeof(struct mgc_chunk_cache_entry));
	cache->gen_mesh_buffer = calloc(1, sizeof(struct chunk_gen_mesh_buffer));

	paged_list_init(
		&cache->chunk_pool,
		memory,
		sizeof(struct mgc_chunk_pool_entry)
	);

	mgc_chunk_spatial_index_init(&cache->index, memory);
}

static struct mgc_chunk *
mgc_chunk_cache_alloc_chunk(struct mgc_chunk_cache *cache)
{
	struct mgc_chunk_pool_entry *entry;
	entry = cache->chunk_pool_free_list;
	if (entry) {
		cache->chunk_pool_free_list = entry->next;
		entry->next = NULL;
		return &entry->chunk;
	}

	size_t id = paged_list_push(&cache->chunk_pool);
	entry = paged_list_get(&cache->chunk_pool, id);
	entry->id = id;

	return &entry->chunk;
}

isize
mgc_chunk_cache_find(struct mgc_chunk_cache *cache, v3i coord)
{
	u32 result;
	result = mgc_chunk_spatial_index_get(&cache->index, coord);
	if (result == MGC_CHUNK_SPATIAL_INDEX_NO_CHUNK) {
		return -1;
	}

	assert(
		cache->entries[result].coord.x == coord.x &&
		cache->entries[result].coord.y == coord.y &&
		cache->entries[result].coord.z == coord.z
	);

	return result;
}

void
mgc_chunk_cache_request(struct mgc_chunk_cache *cache, v3i coord)
{
	isize chunk_i;
	chunk_i = mgc_chunk_cache_find(cache, coord);

	if (chunk_i >= 0) {
		return;
	}

	// TODO: Implement an eviction strategy.
	assert(cache->head < cache->cap_entries);

	chunk_i = cache->head;
	cache->head += 1;

	struct mgc_chunk_cache_entry *entry;
	entry = &cache->entries[chunk_i];
	memset(&cache->entries[chunk_i], 0, sizeof(struct mgc_chunk_cache_entry));

	entry->state = MGC_CHUNK_CACHE_UNLOADED;
	entry->coord = coord;

	mgc_chunk_spatial_index_insert(&cache->index, coord, chunk_i);
}

void
mgc_chunk_cache_invalidate(struct mgc_chunk_cache *cache, v3i coord)
{
	size_t chunk_i;
	chunk_i = mgc_chunk_cache_find(cache, coord);

	assert(chunk_i >= 0);

	struct mgc_chunk_cache_entry *entry;
	entry = &cache->entries[chunk_i];

	assert(entry->state != MGC_CHUNK_CACHE_UNUSED);

	if (entry->state == MGC_CHUNK_CACHE_MESHED) {
		entry->state = MGC_CHUNK_CACHE_DIRTY;
	}
}

void
mgc_chunk_cache_set_sim_center(struct mgc_chunk_cache *cache, v3i sim_center)
{
	cache->sim_center = sim_center;

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
}

void
mgc_chunk_cache_tick(struct mgc_chunk_cache *cache)
{
	TracyCZone(trace, true);

	mgc_world_tick(cache->world);

	for (size_t entry_i = 0; entry_i < cache->head; entry_i++) {
		struct mgc_chunk_cache_entry *entry = &cache->entries[entry_i];

		int err;

		switch (entry->state) {
			case MGC_CHUNK_CACHE_UNUSED:
				break;

			case MGC_CHUNK_CACHE_UNLOADED:
				mgccc_debug_trace(entry->coord, "Loading...");
				assert(entry->chunk == NULL);
				entry->chunk = mgc_chunk_cache_alloc_chunk(cache);
				err = mgc_world_load_chunk(cache->world, entry->chunk, entry->coord);
				if (err < 0) {
					entry->state = MGC_CHUNK_CACHE_FAILED;
					mgccc_debug_trace(entry->coord, "Loading FAILED");
					break;
				} else if (err > 0) {
					entry->state = MGC_CHUNK_CACHE_UNLOADED;
					mgccc_debug_trace(entry->coord, "Loading YIELD");
					break;
				}
				entry->state = MGC_CHUNK_CACHE_LOADED;
				mgccc_debug_trace(entry->coord, "Loading OK");
				break;

			case MGC_CHUNK_CACHE_LOADED:
			case MGC_CHUNK_CACHE_DIRTY:
			case MGC_CHUNK_CACHE_MESHED:
				break;

			case MGC_CHUNK_CACHE_FAILED:
				break;
		}
	}

	TracyCZoneEnd(trace);
}

void
mgc_chunk_cache_render_tick(struct mgc_chunk_cache *cache)
{
	TracyCZone(trace, true);

	for (size_t entry_i = 0; entry_i < cache->head; entry_i++) {
		struct mgc_chunk_cache_entry *entry = &cache->entries[entry_i];

		switch (entry->state) {
			case MGC_CHUNK_CACHE_UNUSED:
			case MGC_CHUNK_CACHE_UNLOADED:
			case MGC_CHUNK_CACHE_FAILED:
				break;

			case MGC_CHUNK_CACHE_LOADED:
			case MGC_CHUNK_CACHE_DIRTY:
				entry->dirty_mask = UINT64_MAX;
				// fallthrough
			case MGC_CHUNK_CACHE_MESHED:
				if (entry->dirty_mask) {
					mgccc_debug_trace(entry->coord, "Meshing...");
					struct mgc_chunk_gen_mesh_result res = {0};
					res = chunk_gen_mesh(
						cache->gen_mesh_buffer,
						cache->mat_table,
						entry->chunk,
						entry->dirty_mask
					);
					entry->dirty_mask = 0;
					for (size_t i = 0; i < RENDER_CHUNKS_PER_CHUNK; i++) {
						if (res.set[i]) {
							entry->mesh[i] = res.mesh[i];
						}
					}
					entry->state = MGC_CHUNK_CACHE_MESHED;
					mgccc_debug_trace(entry->coord, "Meshing OK");
					// fallthrough
				}
				break;
		}
	}

	TracyCZoneEnd(trace);
}

void
mgc_chunk_cache_make_render_queue(
		struct mgc_chunk_cache *cache,
		size_t queue_cap,
		struct mgc_chunk_render_entry *queue,
		size_t *queue_head)
{
	TracyCZone(trace, true);

	for (size_t i = 0; i < cache->head; i++) {
		assert(*queue_head < queue_cap);

		struct mgc_chunk_cache_entry *entry;
		entry = &cache->entries[i];

		if (entry->state == MGC_CHUNK_CACHE_MESHED ||
			entry->state == MGC_CHUNK_CACHE_DIRTY) {
			for (size_t rchunk_i = 0; rchunk_i < RENDER_CHUNKS_PER_CHUNK; rchunk_i++) {
				if (entry->mesh[rchunk_i].numVertices > 0) {
					v3i chunk_offset = V3i(
						entry->coord.x * CHUNK_WIDTH,
						entry->coord.y * CHUNK_WIDTH,
						entry->coord.z * CHUNK_HEIGHT
					);
					v3i rchunk_offset = V3i(
						(rchunk_i % RENDER_CHUNKS_PER_CHUNK_WIDTH) * RENDER_CHUNK_WIDTH,
						((rchunk_i / RENDER_CHUNKS_PER_CHUNK_WIDTH) % RENDER_CHUNKS_PER_CHUNK_WIDTH) * RENDER_CHUNK_WIDTH,
						(rchunk_i / RENDER_CHUNKS_PER_CHUNK_LAYER) * RENDER_CHUNK_HEIGHT
					);

					assert(*queue_head < queue_cap);
					struct mgc_chunk_render_entry *render_entry;
					render_entry = &queue[*queue_head];

					render_entry->mesh = entry->mesh[rchunk_i];
					render_entry->coord = v3i_add(chunk_offset, rchunk_offset);

					*queue_head += 1;
				}
			}
		}
	}

	TracyCZoneEnd(trace);
}

static struct mgc_chunk_spatial_index_node *
mgc_chunk_spatial_index_alloc(struct mgc_chunk_spatial_index *idx)
{
	if (idx->free_list) {
		struct mgc_chunk_spatial_index_node *result = NULL;
		result = idx->free_list;
		idx->free_list = result->free_list_next;
		memset(result, 0, sizeof(struct mgc_chunk_spatial_index_node));
		return result;
	}

	size_t id;
	id = paged_list_push(&idx->nodes);
	return paged_list_get(&idx->nodes, id);
}

static void
mgc_chunk_spatial_index_free(struct mgc_chunk_spatial_index *idx, struct mgc_chunk_spatial_index_node *node)
{
	memset(node, 0, sizeof(struct mgc_chunk_spatial_index_node));
	node->free_list_next = idx->free_list;
	idx->free_list = node;
}

#define COORD_OFFSET 100

static inline v3u
world_to_idx(v3i c)
{
	return V3u(
		c.x + COORD_OFFSET,
		c.y + COORD_OFFSET,
		c.z + COORD_OFFSET
	);
}

static inline v3i
idx_to_world(v3u c)
{
	return V3i(
		c.x - COORD_OFFSET,
		c.y - COORD_OFFSET,
		c.z - COORD_OFFSET
	);
}

#undef COORD_OFFSET

static inline v3u
idx_to_level(v3u c, unsigned int level) 
{
	return V3u(
		c.x >> (MGC_CHUNK_SPATIAL_INDEX_WIDTH_LOG2*level),
		c.y >> (MGC_CHUNK_SPATIAL_INDEX_WIDTH_LOG2*level),
		c.z >> (MGC_CHUNK_SPATIAL_INDEX_WIDTH_LOG2*level)
	);
}

static int
idx_to_child_id(v3u c)
{
	int result = 0;
	result += (c.x % MGC_CHUNK_SPATIAL_INDEX_WIDTH);
	result += (c.y % MGC_CHUNK_SPATIAL_INDEX_WIDTH) * MGC_CHUNK_SPATIAL_INDEX_WIDTH;
	result += (c.z % MGC_CHUNK_SPATIAL_INDEX_WIDTH) * MGC_CHUNK_SPATIAL_INDEX_WIDTH * MGC_CHUNK_SPATIAL_INDEX_WIDTH;
	assert(result < MGC_CHUNK_SPATIAL_INDEX_LEAFS);
	return result;
}

void
mgc_chunk_spatial_index_init(struct mgc_chunk_spatial_index *idx, struct mgc_memory *mem)
{
	memset(idx, 0, sizeof(struct mgc_chunk_spatial_index));
	paged_list_init(&idx->nodes, mem, sizeof(struct mgc_chunk_spatial_index_node));
}

int
mgc_chunk_spatial_index_insert(struct mgc_chunk_spatial_index *idx, v3i chunk_coord, u32 chunk_id)
{
	v3u chunk = world_to_idx(chunk_coord);

	if (!idx->root) {
		struct mgc_chunk_spatial_index_node *new_root;
		new_root = mgc_chunk_spatial_index_alloc(idx);
		new_root->level = 0;
		new_root->coord = idx_to_level(chunk, new_root->level+1);

		for (size_t i = 0; i < MGC_CHUNK_SPATIAL_INDEX_LEAFS; i++) {
			new_root->chunks[i].index = MGC_CHUNK_SPATIAL_INDEX_NO_CHUNK;
		}
		idx->root = new_root;
	}

	while (!v3u_equal(idx->root->coord, idx_to_level(chunk, idx->root->level+1))) {
		struct mgc_chunk_spatial_index_node *new_root;
		new_root = mgc_chunk_spatial_index_alloc(idx);
		new_root->level = idx->root->level + 1;
		new_root->coord = idx_to_level(idx->root->coord, 1);
		int child_id = idx_to_child_id(idx->root->coord);
		new_root->children[child_id] = idx->root;
		idx->root = new_root;
	}

	struct mgc_chunk_spatial_index_node *node;
	node = idx->root;

	while (node->level > 0) {
		v3u child_coord = idx_to_level(chunk, node->level);
		int child_id = idx_to_child_id(child_coord);

		if (!node->children[child_id]) {
			struct mgc_chunk_spatial_index_node *new_node;
			new_node = mgc_chunk_spatial_index_alloc(idx);
			new_node->level = node->level - 1;
			new_node->coord = child_coord;
			node->children[child_id] = new_node;

			if (new_node->level == 0) {
				for (size_t i = 0; i < MGC_CHUNK_SPATIAL_INDEX_LEAFS; i++) {
					new_node->chunks[i].index = MGC_CHUNK_SPATIAL_INDEX_NO_CHUNK;
				}
			}
		}

		node = node->children[child_id];
	}

	assert(node && node->level == 0);

	int child_id = idx_to_child_id(chunk);
	if (node->chunks[child_id].index != MGC_CHUNK_SPATIAL_INDEX_NO_CHUNK) {
		return -1;
	}

	node->chunks[child_id].index = chunk_id;
	return 0;
}

int
mgc_chunk_spatial_index_remove(struct mgc_chunk_spatial_index *idx, v3i chunk_coord)
{
	v3u chunk = world_to_idx(chunk_coord);

	struct mgc_chunk_spatial_index_node *node;
	node = idx->root;

	while (node && node->level > 0) {
		v3u child_coord = idx_to_level(chunk, node->level);
		int child_id = idx_to_child_id(child_coord);

		node = node->children[child_id];
	}

	if (!node || node->level != 0) {
		return -1;
	}

	int child_id = idx_to_child_id(chunk);
	if (node->chunks[child_id].index == MGC_CHUNK_SPATIAL_INDEX_NO_CHUNK) {
		return -1;
	}

	node->chunks[child_id].index = MGC_CHUNK_SPATIAL_INDEX_NO_CHUNK;
	return 0;
}

u32
mgc_chunk_spatial_index_get(struct mgc_chunk_spatial_index *idx, v3i chunk_coord)
{
	v3u chunk = world_to_idx(chunk_coord);

	struct mgc_chunk_spatial_index_node *node;
	node = idx->root;
	if (!node) {
		return MGC_CHUNK_SPATIAL_INDEX_NO_CHUNK;
	}

	while (node && node->level > 0) {
		v3u child_coord = idx_to_level(chunk, node->level);
		int child_id = idx_to_child_id(child_coord);

		node = node->children[child_id];
	}

	if (!node || node->level != 0) {
		return MGC_CHUNK_SPATIAL_INDEX_NO_CHUNK;
	}

	if (!v3u_equal(node->coord, idx_to_level(chunk, 1))) {
		return MGC_CHUNK_SPATIAL_INDEX_NO_CHUNK;
	}

	int child_id = idx_to_child_id(chunk);
	return node->chunks[child_id].index;
}
