#ifndef MAGIC_WORLD_DEF_PATH_H
#define MAGIC_WORLD_DEF_PATH_H

#include "atom.h"
#include "arena.h"

enum mgcd_path_origin {
	MGCD_PATH_ABS,
	MGCD_PATH_REL,
};

struct mgcd_path {
	enum mgcd_path_origin origin;
	struct atom **segments;
	size_t num_segments;
};

int
mgcd_path_parse_str(struct arena *mem, struct atom_table *atom_table, struct string path,
		struct mgcd_path *out);

typedef int mgcd_resource_id;
struct mgcd_context;

int
mgcd_path_make_abs(struct mgcd_context *ctx,
		mgcd_resource_id root_scope, struct mgcd_path path,
		struct mgcd_path *out);

void
mgcd_path_print(struct mgcd_path *);

struct string
mgcd_path_to_string(struct arena *mem, struct mgcd_path *);

#endif
