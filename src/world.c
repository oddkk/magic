#include "world.h"
#include "profile.h"

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
	TracyCZone(trace, true);

	int err = -1;

	switch (world->src) {
		case MGC_WORLD_SRC_WORLD_DEF:
			err = mgc_world_src_world_def_load_chunk(&world->world_def, chunk, coord);
			break;

		case MGC_WORLD_SRC_PRECOMPUTED:
			err = mgc_world_src_precomputed_load_chunk(&world->precomputed, chunk, coord);
			break;

		default:
			panic("Invalid world source.");
			break;
	}

	TracyCZoneEnd(trace);

	return err;
}

void
mgc_world_tick(struct mgc_world *world)
{
	TracyCZone(trace, true);
	switch (world->src) {
		case MGC_WORLD_SRC_WORLD_DEF:
			mgc_world_src_world_def_tick(&world->world_def);
			break;

		case MGC_WORLD_SRC_PRECOMPUTED:
			mgc_world_src_precomputed_tick(&world->precomputed);
			break;
		default:
			panic("Invalid world source.");
			break;
	}

	TracyCZoneEnd(trace);
}
