#include "world_src_world_def.h"
#include "world.h"

#include "world_def/world_def.h"
#include "world_def/jobs.h"

void
mgc_world_src_world_def_init(
		struct mgc_world_src_world_def *world,
		struct mgc_world_init_context *init_ctx)
{
	world->ctx = arena_alloc(init_ctx->world_arena, sizeof(struct mgcd_context));
	mgcd_context_init(
		world->ctx,
		init_ctx->atom_table,
		init_ctx->memory,
		init_ctx->world_arena,
		init_ctx->transient_arena,
		init_ctx->err
	);
}

int
mgc_world_src_world_def_load_chunk(
		struct mgc_world_src_world_def *world,
		struct mgc_chunk *chunk,
		v3i coord)
{

	return 1;
}

void
mgc_world_src_world_def_tick(struct mgc_world_src_world_def *world)
{
	int err;
	err = mgcd_jobs_dispatch(world->ctx);
	print_errors(world->ctx->err);
}
