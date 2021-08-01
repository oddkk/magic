#include "wd_shape.h"
#include "wd_parser.h"
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
			mgcd_var_print_mgcd_resource_id(op->shape.id);
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

static struct mgcd_shape_op *
mgcd_alloc_shape_op(struct mgcd_parser *parser, struct mgcd_shape_op op)
{
	struct mgcd_shape_op *new_op;
	new_op = arena_alloc(parser->ctx->mem, sizeof(struct mgcd_shape_op));

	*new_op = op;

	return new_op;
}

struct mgcd_shape_op *
mgcd_parse_shape_op(struct mgcd_parser *parser)
{
	struct atom *shape_kind;
	struct mgc_location op_loc = {0};
	if (!mgcd_try_eat_ident(parser, &shape_kind, &op_loc)) {
		mgcd_report_unexpect_tok(parser, MGCD_TOK_IDENT);
		return NULL;
	}

	struct mgcd_atoms *atoms;
	atoms = &parser->ctx->atoms;

	struct mgcd_shape_op op = {0};

	if (shape_kind == atoms->shape) {
		// op.kind = MGCD_SHAPE_SHAPE;
		// op.shape.

		panic("TODO: Shape");
	} else if (shape_kind == atoms->cell) {
		op.op = MGCD_SHAPE_CELL;

		if (!mgcd_expect_var_v3i(parser, &op.cell.coord)) {
			return NULL;
		}

	} else if (shape_kind == atoms->heightmap) {
		op.op = MGCD_SHAPE_HEIGHTMAP;

		if (!mgcd_expect_var_resource(parser, MGCD_ENTRY_UNKNOWN, &op.heightmap.file)) {
			return NULL;
		}

		if (!mgcd_expect_var_v3i(parser, &op.heightmap.coord)) {
			return NULL;
		}

	} else if (shape_kind == atoms->hexagon) {
		op.op = MGCD_SHAPE_HEXAGON;

		if (!mgcd_expect_var_v3i(parser, &op.hexagon.center)) {
			return NULL;
		}
		if (!mgcd_expect_var_int(parser, &op.hexagon.radius)) {
			return NULL;
		}
		if (!mgcd_expect_var_int(parser, &op.hexagon.height)) {
			op.hexagon.height = mgcd_var_lit_int(1);
		}

	} else if (shape_kind == atoms->between) {
		op.op = MGCD_SHAPE_BETWEEN;

		op.between.s0 = mgcd_parse_shape_op(parser);
		if (!op.between.s0) {
			return NULL;
		}

		if (!mgcd_expect_tok(parser, MGCD_TOK_ARG_SEP)) {
			mgcd_report_unexpect_tok(parser, MGCD_TOK_ARG_SEP);
			return NULL;
		}

		op.between.s1 = mgcd_parse_shape_op(parser);
		if (!op.between.s1) {
			return NULL;
		}

	} else {
		return NULL;
	}

	return mgcd_alloc_shape_op(parser, op);
}

bool
mgcd_parse_shape_block(
		struct mgcd_parser *parser,
		struct mgcd_shape *shape,
		struct mgcd_shape_op **out_ops)
{
	struct mgcd_shape_op *ops = NULL, **ops_ptr = &ops;

	while (mgcd_block_continue(mgcd_peek_token(parser))) {
		if (mgcd_peek_token(parser).type == MGCD_TOK_END_STMT) {
			mgcd_eat_token(parser);
			continue;
		}

		struct mgcd_shape_op *stmt;
		stmt = mgcd_parse_shape_op(parser);

		if (!stmt) {
			mgcd_stmt_error_recover(parser);
			continue;
		}

		*ops_ptr = stmt;
		ops_ptr = &stmt->next;

		if (!mgcd_expect_tok(parser, MGCD_TOK_END_STMT)) {
			mgcd_report_unexpect_tok(parser, MGCD_TOK_END_STMT);
			mgcd_stmt_error_recover(parser);
			continue;
		}
	}

	*out_ops = ops;
	return true;
}

bool
mgcd_parse_shape_file(
		struct mgcd_context *parse_ctx,
		struct mgcd_lexer *lexer,
		struct mgcd_shape_file *file)
{
	memset(file, 0, sizeof(struct mgcd_shape_file));

	struct mgcd_parser parser = {0};
	mgcd_parse_init(&parser, parse_ctx, lexer);

	struct mgc_location version_loc = {0};
	if (!mgcd_try_eat_version(&parser, &file->version, &version_loc)) {
		mgc_warning(parse_ctx->err, version_loc,
				"Missing version number. Assuming newest version.\n");
	}

	bool result;
	result = mgcd_parse_shape_block(
			&parser, &file->shape, &file->shape.ops);

	return result;
}

void
mgcd_print_shape_file(struct mgcd_shape_file *file)
{
	printf("shape file (file format v%i.%i)\n",
			file->version.major, file->version.minor);
	struct mgcd_shape_op *op = file->shape.ops;

	while (op) {
		mgcd_shape_op_print(op);
		op = op->next;
	}
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

int
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
		return -1;
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

	return 0;
}
