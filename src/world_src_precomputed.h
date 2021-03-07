#ifndef MAGIC_SRC_PRECOMPUTED_H
#define MAGIC_SRC_PRECOMPUTED_H

#include "types.h"
#include "chunk.h"

struct mgc_world_src_precomputed {
	// TODO: Implement precomputed chunks.
	int _dc;
};

struct mgc_world_init_context;
void
mgc_world_src_precomputed_init(struct mgc_world_src_precomputed *, struct mgc_world_init_context *);

int
mgc_world_src_precomputed_load_chunk(
		struct mgc_world_src_precomputed *world,
		struct mgc_chunk *chunk,
		v3i coord);

void
mgc_world_src_precomputed_tick(struct mgc_world_src_precomputed *world);

#endif
