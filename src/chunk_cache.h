#ifndef MAGIC_CHUNK_CACHE_H
#define MAGIC_CHUNK_CACHE_H

#include "types.h"
#include "mesh.h"

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

struct mgc_chunk_cache_entry {
	enum mgc_chunk_cache_entry_state state;
	v3i coord;
	struct mgc_chunk *chunk;
	struct mgc_mesh mesh;
};

struct chunk_gen_mesh_buffer;

struct mgc_chunk_cache {
	// TODO: Spatial data structure
	struct mgc_chunk_cache_entry *entries;
	size_t cap_entries;
	size_t head;

	struct chunk_gen_mesh_buffer *gen_mesh_buffer;
	struct mgc_material_table *mat_table;

	struct mgc_world *world;
};

void
mgc_chunk_cache_init(struct mgc_chunk_cache *, struct mgc_world *, struct mgc_material_table *);

void
mgc_chunk_cache_request(struct mgc_chunk_cache *, v3i coord);

void
mgc_chunk_cache_invalidate(struct mgc_chunk_cache *, v3i coord);

void
mgc_chunk_cache_tick(struct mgc_chunk_cache *cache);

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
