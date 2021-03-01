#include "chunk_cache.h"
#include "config.h"
#include "utils.h"
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
mgc_chunk_cache_init(struct mgc_chunk_cache *cache)
{
	memset(cache, 0, sizeof(struct mgc_chunk_cache));
	cache->cap_entries = MGC_CHUNK_CACHE_SIZE;
	cache->entries = calloc(cache->cap_entries, sizeof(struct mgc_chunk_cache_entry));
}

static ssize_t
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
	for (size_t entry_i = 0; entry_i < cache->head; entry_i++) {
		struct mgc_chunk_cache_entry *entry = &cache->entries[entry_i];

		switch (entry->state) {
			case MGC_CHUNK_CACHE_UNUSED:
				break;

			case MGC_CHUNK_CACHE_UNLOADED:
				mgccc_debug_trace(entry->coord, "Loading...");
				// TODO: Load chunk from world def and saved delta.
				entry->state = MGC_CHUNK_CACHE_LOADED;
				mgccc_debug_trace(entry->coord, "Loading OK");
				// fallthrough

			case MGC_CHUNK_CACHE_LOADED:
			case MGC_CHUNK_CACHE_DIRTY:
				mgccc_debug_trace(entry->coord, "Meshing...");
				// TODO: Mesh
				entry->state = MGC_CHUNK_CACHE_MESHED;
				mgccc_debug_trace(entry->coord, "Meshing OK");
				// fallthrough

			case MGC_CHUNK_CACHE_MESHED:
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
		queue[*queue_head].mesh = cache->entries[i].mesh;
		queue[*queue_head].coord = cache->entries[i].coord;

		*queue_head += 1;
	}
}
