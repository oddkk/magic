#include "shape.h"
#include "utils.h"

size_t
mgc_shape_op_num_children(struct mgc_shape_op *op)
{
	switch (op->op) {
		case MGC_SHAPE_NONE:
			panic("None shape has no children");
			return 0;

		case MGC_SHAPE_CELL:
		case MGC_SHAPE_CUBE:
		case MGC_SHAPE_HEXAGON:
		case MGC_SHAPE_HEIGHTMAP:
			return 0;

		case MGC_SHAPE_BETWEEN:
			return 2;
	}
}

static inline struct mgc_shape_op *
op_next(struct mgc_shape_op **ops_ptr)
{
	assert((*ops_ptr)->op != MGC_SHAPE_NONE);
	struct mgc_shape_op *op = *ops_ptr;
	ops_ptr += 1;
	return op;
}


struct mgc_aabbi
mgc_shape_op_bounds(struct mgc_shape_op **ops_ptr)
{
	struct mgc_aabbi result = {0};
	struct mgc_shape_op *op;
	op = op_next(ops_ptr);

	switch (op->op) {
		case MGC_SHAPE_NONE:
			panic("None shape has no bounds");
			break;

		case MGC_SHAPE_CELL:
			result = mgc_aabbi_from_point(op->cell.coord);
			break;

		case MGC_SHAPE_CUBE:
			result = mgc_aabbi_from_extents(op->cube.p0, op->cube.p1);
			break;

		case MGC_SHAPE_HEXAGON:
			result.min.x = op->hexagon.center.x - op->hexagon.radius+1;
			result.min.y = op->hexagon.center.y - op->hexagon.radius+1;
			result.min.z = op->hexagon.center.z - op->hexagon.radius+1;

			result.max.x = op->hexagon.center.x + op->hexagon.radius;
			result.max.y = op->hexagon.center.y + op->hexagon.radius;
			result.max.z = op->hexagon.center.z + op->hexagon.radius;
			break;

		case MGC_SHAPE_HEIGHTMAP:
			break;

		case MGC_SHAPE_BETWEEN:
			{
				struct mgc_aabbi op1, op2;
				op1 = mgc_shape_op_bounds(ops_ptr);
				op2 = mgc_shape_op_bounds(ops_ptr);

				result = mgc_aabbi_intersect_bounds(op1, op2);
			}
			break;
	}

	return result;
}
