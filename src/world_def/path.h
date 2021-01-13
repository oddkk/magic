#ifndef MAGIC_WORLD_DEF_PATH_H
#define MAGIC_WORLD_DEF_PATH_H

#include "../atom.h"
#include "../arena.h"

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

void
mgcd_path_print(struct mgcd_path *);

#endif
