#include "shape.h"
#include "utils.h"
#include <stdio.h>

void
mgc_world_transform_translate(struct mgc_world_transform *trans, v3i v)
{
	trans->translation = v3i_add(trans->translation, v);
}

void
mgc_world_transform_xy_rotate(struct mgc_world_transform *trans, int r)
{
	m4i rot_matrix;
	mat4_hex_rotate(&rot_matrix, r);

	v4i new_trans = V4i(
		trans->translation.x,
		trans->translation.y,
		trans->translation.z,
		1
	);

	vec4i_multiply_mat4(new_trans.m, new_trans.m, rot_matrix.m);

	trans->translation = V3i(
		new_trans.x,
		new_trans.y,
		new_trans.z
	);
	trans->xy_rotation = (((trans->xy_rotation + r) % 6) + 6) % 6;
}

#define MGC_SHAPE_MAX_NUM_CHILDREN 2

size_t
mgc_shape_op_num_children(struct mgc_shape_op *op)
{
	switch (op->op) {
		case MGC_SHAPE_NONE:
			panic("None shape has no children");
			return 0;

		case MGC_SHAPE_CELL:
		case MGC_SHAPE_HEXAGON:
		case MGC_SHAPE_HEIGHTMAP:
			return 0;

		case MGC_SHAPE_BETWEEN:
			return 2;
	}

	panic("Invalid shape type");
	return 0;
}

static inline struct mgc_shape_op *
op_next(struct mgc_shape *shape, size_t *ops_i)
{
	assert(*ops_i < shape->num_ops);
	struct mgc_shape_op *op = &shape->ops[*ops_i];
	*ops_i += 1;
	return op;
}

// trans is expected to transform the chunk into chunk-local coords.
void
mgc_shape_fill_chunk(
		struct mgc_chunk_mask *chunk,
		struct mgc_world_transform trans,
		struct mgc_shape *shape)
{
	// TODO: Bail if chunk bounds does not intersect with shape.

	size_t op_iter = 0;

	while (op_iter < shape->num_ops) {
		struct mgc_shape_op *op = op_next(shape, &op_iter);

		switch (op->op) {
			case MGC_SHAPE_NONE:
				break;

			case MGC_SHAPE_CELL:
				mgc_chunk_mask_set(
						chunk,
						mgc_world_transform(trans, op->cell.coord));
				break;

			case MGC_SHAPE_HEXAGON:
				{
					v3i center = mgc_world_transform(trans, op->hexagon.center);
					struct mgc_chunk_layer_mask hex_layer = {0};
					int radius = op->hexagon.radius;

					assert(radius >= 0);

					int q1 = center.y - radius;
					int q2 = center.y + radius;

					q1 = max(q1, 0);
					q2 = min(q2, CHUNK_HEIGHT-1);
					for (int q = q1; q <= q2; q++) {
						int y = q - center.y;
						int r1 = center.x + max(-radius, -y - radius);
						int r2 = center.x + min(radius, -y + radius);

						r1 = max(r1, 0);
						r2 = min(r2, CHUNK_HEIGHT-1);

						for (int r = r1; r <= r2; r++) {
							v2i coord;
							coord.x = r;
							coord.y = q;
							mgc_chunk_layer_mask_set(&hex_layer, coord);
						}
					}

					int height = op->hexagon.height;
					int z1 = min(center.z, center.z + height);
					int z2 = max(center.z, center.z + height);

					z1 = max(z1, 0);
					z2 = min(z2, CHUNK_HEIGHT-1);

					for (int z = z1; z <= z2; z++) {
						mgc_chunk_set_layer(chunk, &hex_layer, z);
					}
				}
				break;

			case MGC_SHAPE_HEIGHTMAP:
				break;

			case MGC_SHAPE_BETWEEN:
				break;
		}
	}
}
