#ifndef MAGIC_WORLD_DEF_SHAPE_H
#define MAGIC_WORLD_DEF_SHAPE_H

#include "world_def.h"

typedef int mgcd_shape_id;
typedef int mgcd_heightmap_id;

enum mgcd_shape_op_kind {
	MGCD_SHAPE_SHAPE,
	MGCD_SHAPE_CELL,
	MGCD_SHAPE_HEXAGON,
	MGCD_SHAPE_HEIGHTMAP,
	MGCD_SHAPE_BETWEEN,
};

struct mgcd_shape_op {
	struct mgcd_shape_op *next;
	enum mgcd_shape_op_kind op;
	bool inverted;

	union {
		struct {
			MGCD_TYPE(mgcd_shape_id) id;
			MGCD_TYPE(int) rotation;
			MGCD_TYPE(v3i) translation;
		} shape;

		struct {
			MGCD_TYPE(v3i) coord;
		} cell;

		struct {
			MGCD_TYPE(v3i) p0;
			MGCD_TYPE(v3i) p1;
		} cube;

		struct {
			MGCD_TYPE(v3i) center;
			MGCD_TYPE(int) radius;
			MGCD_TYPE(int) height;
		} hexagon;

		struct {
			struct string file;
			MGCD_TYPE(v3i) coord;
		} heightmap;

		struct {
			struct mgcd_shape_op *s0;
			struct mgcd_shape_op *s1;
		} between;
	};
};

size_t
mgcd_shape_op_num_children(struct mgcd_shape_op *);

void
mgcd_shape_op_print(struct mgcd_shape_op *);

struct mgcd_var_def {
	struct atom *name;
};

struct mgcd_shape {
	struct mgcd_shape_op *ops;

	struct mgcd_var_def *vars;
	size_t num_vars;
};

struct mgcd_shape_op *
mgcd_parse_shape_op(struct mgcd_parser *);

struct mgcd_shape_file {
	struct mgcd_version version;
	struct mgcd_shape shape;
};

bool
mgcd_parse_shape_file(
		struct mgcd_context *parse_ctx,
		struct mgcd_lexer *lexer,
		struct mgcd_shape_file *file);

void
mgcd_print_shape_file(struct mgcd_shape_file *file);

bool
mgcd_complete_shape(struct mgcd_context *,
		struct mgcd_shape *shape,
		struct arena *mem,
		struct mgc_shape *out_shape);

#endif
