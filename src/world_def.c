#include "world_def.h"
#include "utils.h"

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
	type mgcd_var_value_##name(struct mgcd_var_##type val) { \
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
	switch (op->kind) {
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
	switch (op->kind) {
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

		case MGCD_SHAPE_CUBE:
			printf("CUBE");
			printf(" p0=");
			mgcd_var_print_v3i(op->cube.p0);
			printf(" p1=");
			mgcd_var_print_v3i(op->cube.p1);
			break;

		case MGCD_SHAPE_HEXAGON:
			printf("HEXAGON");
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
