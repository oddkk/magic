#include "world_def.h"
#include "vars.h"
#include "shape.h"
#include "jobs.h"
#include "../utils.h"
#include <string.h>

void
mgcd_world_init(struct mgcd_world *world, struct mgc_memory *mem)
{
	paged_list_init(&world->shapes, mem,
			sizeof(struct mgcd_shape_op));
	paged_list_init(&world->shape_ops, mem,
			sizeof(struct mgcd_shape_op));
}

void
mgcd_context_init(struct mgcd_context *ctx,
		struct mgcd_world *world,
		struct atom_table *atom_table,
		struct mgc_memory *memory,
		struct arena *mem,
		struct arena *tmp_mem,
		struct mgc_error_context *err)
{
	ctx->world = world;
	ctx->atom_table = atom_table;
	ctx->mem = mem;
	ctx->tmp_mem = tmp_mem;
	ctx->err = err;

	// Initialize common atoms.
#define ATOM(name) ctx->atoms.name = atom_create(ctx->atom_table, STR(#name));
	MGCD_ATOMS
#undef ATOM

	mgcd_context_jobs_init(ctx, memory);
}
