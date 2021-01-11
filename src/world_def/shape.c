#include "shape.h"
#include <string.h>

size_t
mgcd_shape_op_num_children(struct mgcd_shape_op *op)
{
	switch (op->op) {
		case MGCD_SHAPE_BETWEEN:
			return 2;

		default:
			return 0;
	}
}

void
mgcd_shape_op_print(struct mgcd_shape_op *op)
{
	switch (op->op) {
		case MGCD_SHAPE_SHAPE:
			printf("SHAPE");
			printf(" id=");
			mgcd_var_print_mgcd_shape_id(op->shape.id);
			printf(" rotation=");
			mgcd_var_print_int(op->shape.rotation);
			printf(" translation=");
			mgcd_var_print_v3i(op->shape.translation);
			break;

		case MGCD_SHAPE_CELL:
			printf("CELL");
			printf(" coord=");
			mgcd_var_print_v3i(op->cell.coord);
			break;

		case MGCD_SHAPE_HEXAGON:
			printf("HEXAGON");
			printf(" center=");
			mgcd_var_print_v3i(op->hexagon.center);
			printf(" radius=");
			mgcd_var_print_int(op->hexagon.radius);
			printf(" height=");
			mgcd_var_print_int(op->hexagon.height);
			break;

		case MGCD_SHAPE_HEIGHTMAP:
			printf("HEIGHTMAP");
			break;

		case MGCD_SHAPE_BETWEEN:
			printf("BETWEEN");
			break;
	}

	printf("\n");
}

struct mgcd_tmp_shape_op {
	struct mgcd_tmp_shape_op *next;
	struct mgc_shape_op op;
};

struct mgcd_tmp_shape {
	struct mgcd_tmp_shape_op *head;
	struct mgcd_tmp_shape_op *foot;

	bool error;
};

static struct mgcd_tmp_shape_op *
mgcd_alloc_tmp_op(struct mgcd_context *ctx, struct mgc_shape_op op)
{
	struct mgcd_tmp_shape_op *result;
	result = arena_alloc(ctx->tmp_mem, sizeof(struct mgcd_tmp_shape_op));
	result->next = NULL;
	result->op = op;
	return result;
}

static void
mgcd_append_tmp_op(struct mgcd_tmp_shape *shape, struct mgcd_tmp_shape_op *op)
{
	assert(op->next == NULL);
	if (!shape->head) {
		assert(!shape->foot);
		shape->head = shape->foot = op;
	} else {
		assert(shape->foot->next == NULL);
		shape->foot->next = op;
		shape->foot = op;
	}
}

static void
mgcd_tmp_append_op(
		struct mgcd_context *ctx,
		struct mgcd_tmp_shape *shape,
		struct mgc_shape_op op)
{
	mgcd_append_tmp_op(shape, mgcd_alloc_tmp_op(ctx, op));
}

static struct mgcd_tmp_shape
mgcd_complete_shape_internal(
		struct mgcd_context *ctx,
		struct mgcd_scope *scope,
		struct mgcd_shape *shape,
		struct mgc_world_transform transform)
{
	struct mgcd_tmp_shape result = {0};
	struct mgcd_shape_op *op;
	op = shape->ops;

	while (op) {
		switch (op->op) {
			case MGCD_SHAPE_SHAPE:
				break;

			case MGCD_SHAPE_CELL:
				{
					struct mgc_shape_op res = {0};
					res.op = MGC_SHAPE_CELL;
					v3i coord = mgcd_var_value_v3i(scope, op->cell.coord);
					res.cell.coord = mgc_world_transform(transform, coord);
					mgcd_tmp_append_op(ctx, &result, res);
				}
				break;

			case MGCD_SHAPE_HEXAGON:
				{
					struct mgc_shape_op res = {0};
					res.op = MGC_SHAPE_HEXAGON;
					v3i center = mgcd_var_value_v3i(scope, op->hexagon.center);

					res.hexagon.center = mgc_world_transform(transform, center);
					res.hexagon.radius = mgcd_var_value_int(scope, op->hexagon.radius);
					res.hexagon.height = mgcd_var_value_int(scope, op->hexagon.height);


					if (res.hexagon.radius < 0) {
						mgc_error(ctx->err, MGC_NO_LOC,
								"Hexagon radius must be >= 0. Got %i.",
								res.hexagon.radius);
						result.error = true;
						break;
					}

					mgcd_tmp_append_op(ctx, &result, res);
				}
				break;

			case MGCD_SHAPE_HEIGHTMAP:
				break;

			case MGCD_SHAPE_BETWEEN:
				break;
		}

		op = op->next;
	}

	return result;
}

bool
mgcd_complete_shape(struct mgcd_context *ctx,
		struct mgcd_shape *shape,
		struct arena *mem,
		struct mgc_shape *out_shape)
{
	arena_mark cp = arena_checkpoint(ctx->tmp_mem);

	struct mgcd_tmp_shape result;
	result = mgcd_complete_shape_internal(
			ctx, NULL, shape, (struct mgc_world_transform){0});

	if (result.error) {
		arena_reset(ctx->tmp_mem, cp);
		return false;
	}

	size_t num_ops = 0;
	struct mgcd_tmp_shape_op *ops_iter = result.head;
	while (ops_iter) {
		num_ops += 1;
		ops_iter = ops_iter->next;
	}

	struct mgc_shape_op *final_ops;
	final_ops = arena_allocn(mem, num_ops, sizeof(struct mgc_shape_op));

	size_t ops_i = 0;
	ops_iter = result.head;
	while (ops_iter) {
		final_ops[ops_i] = ops_iter->op;

		ops_i += 1;
		ops_iter = ops_iter->next;
	}

	assert(ops_i == num_ops);

	arena_reset(ctx->tmp_mem, cp);

	memset(out_shape, 0, sizeof(struct mgc_shape));
	out_shape->ops = final_ops;
	out_shape->num_ops = num_ops;
	return false;
}
