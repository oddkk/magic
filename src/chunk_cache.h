#ifndef MAGIC_CHUNK_CACHE_H
#define MAGIC_CHUNK_CACHE_H

#include "intdef.h"
#include "types.h"
#include "mesh.h"
#include "arena.h"
#include "chunk.h"

struct mgc_chunk;
struct mgc_world;

enum mgc_chunk_cache_entry_state {
	MGC_CHUNK_CACHE_UNUSED = 0,
	MGC_CHUNK_CACHE_UNLOADED,
	MGC_CHUNK_CACHE_LOADED,
	MGC_CHUNK_CACHE_MESHED,
	MGC_CHUNK_CACHE_DIRTY,
	MGC_CHUNK_CACHE_FAILED,
};

// For simplicity, only allow max 64 render chunks per chunk in order to fit a
// bit mask into a u64.
#if RENDER_CHUNKS_PER_CHUNK > 64
#error "RENDER_CHUNKS_PER_CHUNK must be less than 64"
#endif

struct mgc_chunk_cache_entry {
	enum mgc_chunk_cache_entry_state state;
	v3i coord;
	struct mgc_chunk *chunk;
	struct mgc_mesh mesh[RENDER_CHUNKS_PER_CHUNK];
	u64 dirty_mask;
};

struct chunk_gen_mesh_buffer;

struct mgc_chunk_pool_entry {
	size_t id;
	struct mgc_chunk_pool_entry *next;
	struct mgc_chunk chunk;
};

struct mgc_chunk_spatial_index_entry {
	u32 index;
};

#define MGC_CHUNK_SPATIAL_INDEX_WIDTH_LOG2 (1)
#define MGC_CHUNK_SPATIAL_INDEX_WIDTH (1 << MGC_CHUNK_SPATIAL_INDEX_WIDTH_LOG2)
#define MGC_CHUNK_SPATIAL_INDEX_CHILDREN (MGC_CHUNK_SPATIAL_INDEX_WIDTH*MGC_CHUNK_SPATIAL_INDEX_WIDTH*MGC_CHUNK_SPATIAL_INDEX_WIDTH)
#define MGC_CHUNK_SPATIAL_INDEX_LEAFS MGC_CHUNK_SPATIAL_INDEX_CHILDREN
/*
(( \
		sizeof(struct mgc_chunk_spatial_index_node *) * \
		MGC_CHUNK_SPATIAL_INDEX_CHILDREN) / \
		sizeof(struct mgc_chunk_spatial_index_entry))
		*/

// The coordinate system for this index is translated to (INT_MAX, INT_MAX,
// INT_MAX). This is to avoid having to deal with rounding negative integers
// towards negative infinity instead of towards 0.
struct mgc_chunk_spatial_index_node {
	v3u coord;
	unsigned int level;
	// size_t num_chunks;
	union {
		struct mgc_chunk_spatial_index_entry chunks[MGC_CHUNK_SPATIAL_INDEX_LEAFS];
		struct mgc_chunk_spatial_index_node *children[MGC_CHUNK_SPATIAL_INDEX_CHILDREN];
		struct mgc_chunk_spatial_index_node *free_list_next;
	};
};

struct mgc_chunk_spatial_index {
	struct mgc_chunk_spatial_index_node *root;
	struct mgc_chunk_spatial_index_node *free_list;
	struct paged_list nodes;
};

#define MGC_CHUNK_SPATIAL_INDEX_NO_CHUNK UINT32_MAX

void
mgc_chunk_spatial_index_init(struct mgc_chunk_spatial_index *, struct mgc_memory *);

int
mgc_chunk_spatial_index_insert(struct mgc_chunk_spatial_index *, v3i coord, u32 chunk_id);

int
mgc_chunk_spatial_index_remove(struct mgc_chunk_spatial_index *, v3i coord);

u32
mgc_chunk_spatial_index_get(struct mgc_chunk_spatial_index *, v3i coord);

struct mgc_chunk_cache {
	struct mgc_chunk_cache_entry *entries;
	size_t cap_entries;
	size_t head;

	struct mgc_chunk_spatial_index index;

	struct chunk_gen_mesh_buffer *gen_mesh_buffer;
	struct mgc_material_table *mat_table;

	struct paged_list chunk_pool;
	struct mgc_chunk_pool_entry *chunk_pool_free_list;

	struct mgc_world *world;
};

void
mgc_chunk_cache_init(
		struct mgc_chunk_cache *cache,
		struct mgc_memory *memory,
		struct mgc_world *world,
		struct mgc_material_table *mat_table);

void
mgc_chunk_cache_request(struct mgc_chunk_cache *, v3i coord);

void
mgc_chunk_cache_invalidate(struct mgc_chunk_cache *, v3i coord);

void
mgc_chunk_cache_tick(struct mgc_chunk_cache *cache);

void
mgc_chunk_cache_render_tick(struct mgc_chunk_cache *cache);

isize
mgc_chunk_cache_find(struct mgc_chunk_cache *cache, v3i coord);

struct mgc_chunk_render_entry {
	struct mgc_mesh mesh;
	v3i coord;
};

void
mgc_chunk_cache_make_render_queue(
		struct mgc_chunk_cache *,
		size_t queue_cap,
		struct mgc_chunk_render_entry *queue,
		size_t *queue_head);

#endif
