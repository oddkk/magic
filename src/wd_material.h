#ifndef MAGIC_WORLD_DEF_MATERIAL_H
#define MAGIC_WORLD_DEF_MATERIAL_H

#include "wd_world_def.h"
#include "material.h"

struct mgcd_material {
	struct atom *name;
	struct mgc_location name_loc;
};

struct mgcd_parser;
struct mgcd_lexer;

bool
mgcd_parse_material_block(
		struct mgcd_parser *parser,
		struct mgcd_material *mat);

#endif
