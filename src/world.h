#ifndef MAGIC_WORLD_H
#define MAGIC_WORLD_H

#include "world_src_world_def.h"
#include "world_src_precomputed.h"
#include "types.h"

enum mgc_world_source {
	MGC_WORLD_SRC_WORLD_DEF,
	MGC_WORLD_SRC_PRECOMPUTED,
};

struct mgc_world {
	enum mgc_world_source src;

	struct mgc_material_table *mat_table;

	union {
		struct mgc_world_src_world_def world_def;
		struct mgc_world_src_precomputed precomputed;
	};
};

struct atom_table;
struct mgc_memory;
struct mgc_error_context;
struct mgc_material_table;
struct arena;

struct mgc_world_init_context {
	struct atom_table *atom_table;
	struct mgc_memory *memory;
	struct arena *world_arena;
	struct arena *transient_arena;
	struct mgc_registry *registry;
	struct mgc_error_context *err;
};

void
mgc_world_init_world_def(struct mgc_world *, struct mgc_world_init_context *);

void
mgc_world_init_precomputed(struct mgc_world *, struct mgc_world_init_context *);

int
mgc_world_load_chunk(struct mgc_world *, struct mgc_chunk *, v3i coord);

void
mgc_world_tick(struct mgc_world *);

#endif
