#ifndef MAGIC_SHAPE_H
#define MAGIC_SHAPE_H

#include "types.h"
#include "chunk.h"
#include "chunk_mask.h"
#include "utils.h"

struct mgc_world_transform {
	v3i translation;
	int xy_rotation;
};

void
mgc_world_transform_translate(struct mgc_world_transform *, v3i);

// r is clock-wise rotation in 60 degree increments.
void
mgc_world_transform_xy_rotate(struct mgc_world_transform *, int);

// transforms axial coordinate according to world transform.
static inline v3i
mgc_world_transform(struct mgc_world_transform trans, v3i p)
{
	int u = p.x, v = p.y, w = -p.x-p.y;

	// normalize r from 0 to, excluding, 6.
	int r = ((trans.xy_rotation % 6) + 6) % 6;
	int sign = (r % 2) ? -1 : 1;

	int p0 = ((r % 3) == 0) * sign;
	int p1 = ((r % 3) == 1) * sign;
	int p2 = ((r % 3) == 2) * sign;

	v3i result;

	result.x = u*p0 + v*p1 + w*p2 + trans.translation.x;
	result.y = u*p2 + v*p0 + w*p1 + trans.translation.y;
	result.z = p.z + trans.translation.z;

	return result;
}

enum mgc_shape_op_kind {
	MGC_SHAPE_NONE = 0,

	MGC_SHAPE_CELL,
	MGC_SHAPE_HEXAGON,
	MGC_SHAPE_HEIGHTMAP,
	MGC_SHAPE_BETWEEN,
};

enum mgc_shape_op_ {
	MGC_SHAPE_OP_NONE = 0,
	MGC_SHAPE_OP_UNION,
	MGC_SHAPE_OP_UNION_INVERTED,
	MGC_SHAPE_OP_INTERSECT,
	MGC_SHAPE_OP_INTERSECT_INVERTED,

	MGC_SHAPE_OP_PUSH,
	MGC_SHAPE_OP_PUSH_INVERTED,
};

struct mgc_shape_op {
	enum mgc_shape_op_kind op;
	bool inverted;

	union {
		struct {
			v3i coord;
		} cell;

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
};

size_t
mgc_shape_op_num_children(struct mgc_shape_op *);

void
mgc_shape_fill_chunk(
		struct mgc_chunk_mask *chunk,
		struct mgc_world_transform trans,
		struct mgc_shape *shape);

#endif
