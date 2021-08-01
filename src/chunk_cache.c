#include "chunk_cache.h"
#include "config.h"
#include "utils.h"
#include "world.h"
#include "render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

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

ssize_t
mgc_chunk_cache_find(struct mgc_chunk_cache *cache, v3i coord)
{
	for (size_t i = 0; i < cache->head; i++) {
		if (cache->entries[i].state != MGC_CHUNK_CACHE_UNUSED &&
				cache->entries[i].coord.x == coord.x &&
				cache->entries[i].coord.y == coord.y &&
				cache->entries[i].coord.z == coord.z) {
			return i;
		}
	}

	return -1;
}

void
mgc_chunk_cache_request(struct mgc_chunk_cache *cache, v3i coord)
{
	ssize_t chunk_i;
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
mgc_chunk_cache_tick(struct mgc_chunk_cache *cache)
{
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
				// fallthrough

			case MGC_CHUNK_CACHE_LOADED:
			case MGC_CHUNK_CACHE_DIRTY:
				{
					mgccc_debug_trace(entry->coord, "Meshing...");
					struct mgc_chunk_gen_mesh_result res = {0};
					res = chunk_gen_mesh(
							cache->gen_mesh_buffer,
							cache->mat_table,
							entry->chunk
							);
					for (size_t i = 0; i < RENDER_CHUNKS_PER_CHUNK; i++) {
						entry->mesh[i] = res.mesh[i];
					}
					entry->state = MGC_CHUNK_CACHE_MESHED;
					mgccc_debug_trace(entry->coord, "Meshing OK");
					// fallthrough
				}

			case MGC_CHUNK_CACHE_MESHED:
			case MGC_CHUNK_CACHE_FAILED:
				break;
		}
	}
}

void
mgc_chunk_cache_make_render_queue(
		struct mgc_chunk_cache *cache,
		size_t queue_cap,
		struct mgc_chunk_render_entry *queue,
		size_t *queue_head)
{
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
}
