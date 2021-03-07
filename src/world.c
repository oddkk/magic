#include "world.h"

void
mgc_world_init_world_def(struct mgc_world *world, struct mgc_world_init_context *init_ctx)
{
	mgc_world_src_world_def_init(&world->world_def, init_ctx);
}

void
mgc_world_init_precomputed(struct mgc_world *world, struct mgc_world_init_context *init_ctx)
{
	mgc_world_src_precomputed_init(&world->precomputed, init_ctx);
}

int
mgc_world_load_chunk(struct mgc_world *world, struct mgc_chunk *chunk, v3i coord)
{
	switch (world->src) {
		case MGC_WORLD_SRC_WORLD_DEF:
			return mgc_world_src_world_def_load_chunk(&world->world_def, chunk, coord);

		case MGC_WORLD_SRC_PRECOMPUTED:
			return mgc_world_src_precomputed_load_chunk(&world->precomputed, chunk, coord);
	}

	panic("Invalid world source.");
	return -1;
}

void
mgc_world_tick(struct mgc_world *world)
{
	switch (world->src) {
		case MGC_WORLD_SRC_WORLD_DEF:
			mgc_world_src_world_def_tick(&world->world_def);
			break;

		case MGC_WORLD_SRC_PRECOMPUTED:
			mgc_world_src_precomputed_tick(&world->precomputed);
			break;
	}

	panic("Invalid world source.");
}
