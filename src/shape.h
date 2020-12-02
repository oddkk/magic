#ifndef MAGIC_SHAPE_H
#define MAGIC_SHAPE_H

#include "types.h"

enum mgc_shape_op_kind {
	MGC_SHAPE_NONE = 0,

	MGC_SHAPE_CELL,
	MGC_SHAPE_CUBE,
	MGC_SHAPE_HEXAGON,
	MGC_SHAPE_HEIGHTMAP,
	MGC_SHAPE_BETWEEN,
};

struct mgc_shape_op {
	enum mgc_shape_op_kind op;
	bool inverted;

	union {
		struct {
			v3i coord;
		} cell;

		struct {
			v3i p0;
			v3i p1;
		} cube;

		struct {
			v3i center;
			int radius;
			int height;
		} hexagon;

		struct {
			v3i coord;
		} heightmap;
	};
};

struct mgc_shape {
	struct mgc_shape_op *ops;
	size_t num_ops;

	struct mgc_aabbi bounds;
};

size_t
mgc_shape_op_num_children(struct mgc_shape_op *);

struct mgc_aabbi
mgc_shape_op_bounds(struct mgc_shape_op **ops);

#endif
