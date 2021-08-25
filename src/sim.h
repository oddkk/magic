#ifndef MAGIC_SIM_H
#define MAGIC_SIM_H

#include "chunk_cache.h"
#include "registry.h"
#include "types.h"

struct mgc_sim_chunk {
	struct mgc_chunk_cache_entry *cache_entry;
	struct mgc_chunk_ref neighbours[27];
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
