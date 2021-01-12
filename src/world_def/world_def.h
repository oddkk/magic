#ifndef MAGIC_WORLD_DEF_H
#define MAGIC_WORLD_DEF_H

#include "../types.h"
#include "../atom.h"
#include "../str.h"
#include "../errors.h"
#include "vars.h"
#include <stdio.h>

typedef int mgcd_job_id;

struct mgcd_world {
	struct paged_list shapes;
	struct paged_list shape_ops;
};

struct mgcd_version {
	int major, minor;
};

void
mgcd_world_init(struct mgcd_world *world, struct mgc_memory *mem);

#define MGCD_ATOMS \
	ATOM(shape) \
	ATOM(cell) \
	ATOM(heightmap) \
	ATOM(hexagon) \
	ATOM(between) \
	ATOM(coord) \
	ATOM(file) \
	ATOM(center) \
	ATOM(radius) \
	ATOM(height) \

struct mgcd_atoms {
#define ATOM(name) struct atom *name;
	MGCD_ATOMS
#undef ATOM
};

struct mgcd_context {
	struct mgcd_world *world;
	struct mgcd_atoms atoms;

	struct atom_table *atom_table;
	struct arena *mem;
	struct arena *tmp_mem;

	struct paged_list jobs;
	mgcd_job_id free_list;
	mgcd_job_id terminal_jobs;
	size_t unvisited_job_deps;

	struct mgc_error_context *err;

	size_t run_i;
};

void
mgcd_context_init(struct mgcd_context *,
		struct mgcd_world *world,
		struct atom_table *atom_table,
		struct mgc_memory *memory,
		struct arena *mem,
		struct arena *tmp_mem,
		struct mgc_error_context *err);

#endif
