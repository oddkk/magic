#ifndef MAGIC_SIM_H
#define MAGIC_SIM_H

#include "chunk_cache.h"
#include "registry.h"
#include "types.h"

#define NEIGHBOURHOOD_WIDTH 3
#define NEIGHBOURHOOD_SIZE (NEIGHBOURHOOD_WIDTH*NEIGHBOURHOOD_WIDTH*NEIGHBOURHOOD_WIDTH)
#define NEIGHBOURHOOD_CENTER_IDX 13

#if (RENDER_CHUNKS_PER_CHUNK <= 8)
typedef u8 chunk_dirty_mask_t;
#elif (RENDER_CHUNKS_PER_CHUNK <= 16)
typedef u16 chunk_dirty_mask_t;
#elif (RENDER_CHUNKS_PER_CHUNK <= 32)
typedef u32 chunk_dirty_mask_t;
#elif (RENDER_CHUNKS_PER_CHUNK <= 64)
typedef u64 chunk_dirty_mask_t;
#else
#error "Can not fit render chunks in dirty mask type."
#endif

struct mgc_sim_chunk {
	struct mgc_chunk_cache_entry *cache_entry[27];
	struct mgc_chunk_ref neighbours[27];
	struct chunk_dirty_mask_t *neighbour_dirty_mask[27];
};


struct mgc_sim_buffer {
	struct mgc_sim_chunk sim_chunks[NUM_SIM_CHUNKS];
};

void
mgc_sim_tick(
		struct mgc_sim_buffer *,
		struct mgc_chunk_cache *cache,
		struct mgc_registry *reg,
		u64 sim_tick);

#endif
