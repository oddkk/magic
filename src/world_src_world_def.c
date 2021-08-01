#include "world_src_world_def.h"
#include "world.h"
#include "area.h"

#include "wd_world_def.h"
#include "wd_jobs.h"
#include "wd_material.h"

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
		init_ctx->registry,
		init_ctx->err
	);

	world->terrain_id = mgcd_request_resource_str(
		world->ctx,
		MGCD_JOB_NONE,
		MGC_NO_LOC,
		MGCD_RESOURCE_NONE,
		STR("/world/world")
	);

	world->registry = init_ctx->registry;

	world->files_dirty = true;
}

int
mgc_world_src_world_def_load_chunk(
		struct mgc_world_src_world_def *world,
		struct mgc_chunk *chunk,
		v3i coord)
{
	chunk->location = coord;
	if (!world->terrain) {
		return -1;
	}

	mgc_area_apply(world->terrain, chunk);

	return 0;
}

void
mgc_world_src_world_def_tick(struct mgc_world_src_world_def *world)
{
	if (world->files_dirty) {
		world->files_dirty = false;

		int err;
		err = mgcd_jobs_dispatch(world->ctx);

		if (err) {
			print_errors(world->ctx->err);
			return;
		}

		// TODO: Check if the terrain definiton has changed.
		struct mgcd_resource *res;
		res = mgcd_resource_get(world->ctx, world->terrain_id);

		if (res->type != MGCD_ENTRY_AREA) {
			mgc_error(world->ctx->err, MGC_NO_LOC, "Expected main '/world/world' to be an area.");
			print_errors(world->ctx->err);
			return;
		}

		// TODO: Invalidate chunks that have changed.
		world->terrain = res->area.ready;

		print_errors(world->ctx->err);
	}
}
