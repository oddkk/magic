#ifndef MAGIC_SRC_WORLD_DEF_H
#define MAGIC_SRC_WORLD_DEF_H

#include "types.h"
#include "shape.h"
#include "chunk.h"

typedef int mgcd_resource_id;
struct mgcd_context;
struct mgc_registry;

struct mgc_world_src_world_def {
	struct mgcd_context *ctx;
	struct mgc_registry *registry;

	mgcd_resource_id terrain_id;
	struct mgc_area *terrain;

	bool files_dirty;
};

struct mgc_world_init_context;
void
mgc_world_src_world_def_init(struct mgc_world_src_world_def *, struct mgc_world_init_context *);

void
mgc_world_src_world_def_tick(struct mgc_world_src_world_def *world);

int
mgc_world_src_world_def_load_chunk(
		struct mgc_world_src_world_def *world,
		struct mgc_chunk *chunk,
		v3i coord);

#endif
