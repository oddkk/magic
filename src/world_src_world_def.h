#ifndef MAGIC_SRC_WORLD_DEF_H
#define MAGIC_SRC_WORLD_DEF_H

#include "types.h"
#include "shape.h"
#include "chunk.h"

typedef int mgcd_resource_id;
struct mgcd_context;

struct mgc_terrain_shape {
	struct mgc_shape shape;
	mgc_material_id mat_id;

	mgcd_resource_id resource_id;
};

struct mgc_world_src_world_def {
	struct mgc_terrain_shape *terrain_shapes;
	size_t num_terrain_shapes;

	struct mgcd_context *ctx;
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
