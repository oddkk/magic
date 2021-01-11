#include "world_def.h"
#include "../utils.h"
#include <string.h>

static struct string mgcd_type_names[] = {
#define TYPE(name, type) STR(#name),
MGCD_DEF_VAR_TYPES
#undef TYPE
};

struct string
mgcd_type_name(enum mgcd_type type)
{
	assert(type < MGCD_NUM_TYPES);
	return mgcd_type_names[type];
}

#define TYPE(name, type) \
	type mgcd_var_value_##name(struct mgcd_scope *scope, struct mgcd_var_##type val) { \
		if (val.var == MGCD_VAR_LIT) { \
			return val.litteral; \
		} else { \
			panic("TODO: mgcd variables"); \
			struct mgcd_var_slot slot = {0}; \
			return slot.val_##name; \
		} \
	} \
	struct mgcd_var_##type mgcd_var_lit_##name(type value) { \
		struct mgcd_var_##type result = {0}; \
		result.var = MGCD_VAR_LIT; \
		result.litteral = value; \
		return result; \
	} \
	struct mgcd_var_##type mgcd_var_var_##name(mgcd_var_id id) { \
		struct mgcd_var_##type result = {0}; \
		result.var = id; \
		return result; \
	}
MGCD_DEF_VAR_TYPES
#undef TYPE

void
mgcd_world_init(struct mgcd_world *world, struct mgc_memory *mem)
{
	paged_list_init(&world->shapes, mem,
			sizeof(struct mgcd_shape_op));
	paged_list_init(&world->shape_ops, mem,
			sizeof(struct mgcd_shape_op));
}

void
mgcd_context_init(struct mgcd_context *ctx,
		struct mgcd_world *world,
		struct atom_table *atom_table,
		struct arena *mem,
		struct arena *tmp_mem,
		struct mgc_error_context *err)
{
	ctx->world = world;
	ctx->atom_table = atom_table;
	ctx->mem = mem;
	ctx->tmp_mem = tmp_mem;
	ctx->err = err;

	// Initialize common atoms.
#define ATOM(name) ctx->atoms.name = atom_create(ctx->atom_table, STR(#name));
	MGCD_ATOMS
#undef ATOM
}

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

bool
mgcd_var_print_var(mgcd_var_id id)
{
	if (id == MGCD_VAR_LIT) {
		return false;
	}

	printf("<var %i>", id);
	return true;
}

#define TYPE(name, type) \
	static void mgcd_lit_print_##name(type); \
	void mgcd_var_print_##name(MGCD_TYPE(name) val) \
	{ \
		if (!mgcd_var_print_var(val.var)) { \
			mgcd_lit_print_##name(val.litteral); \
		} \
	}
MGCD_DEF_VAR_TYPES
#undef TYPE

void
mgcd_lit_print_int(int val)
{
	printf("%i", val);
}

void
mgcd_lit_print_v3i(v3i val)
{
	printf("(%i, %i, %i)", val.x, val.y, val.z);
}

static void
mgcd_lit_print_mgcd_shape_id(mgcd_shape_id val)
{
	printf("<shape %i>", val);
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
