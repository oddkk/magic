#include "area.h"
#include "parser.h"
#include "../arena.h"
#include <string.h>

static struct mgcd_area_op *
mgcd_alloc_area_op(struct mgcd_parser *parser, struct mgcd_area_op op)
{
	struct arena *mem = parser->ctx->mem;

	struct mgcd_area_op *new_op;
	new_op = arena_alloc(mem, sizeof(struct mgcd_area_op));

	*new_op = op;

	return new_op;
}

struct mgcd_area_op *
mgcd_parse_area_op(struct mgcd_parser *parser)
{
	struct atom *op_kind;
	struct mgc_location op_loc = {0};
	if (!mgcd_try_eat_ident(parser, &op_kind, &op_loc)) {
		return NULL;
	}

	struct mgcd_atoms *atoms;
	atoms = &parser->ctx->atoms;

	struct mgcd_area_op op = {0};

	if (op_kind == atoms->add) {
		op.op = MGCD_AREA_ADD;

		if (!mgcd_expect_var_resource(parser, MGCD_ENTRY_MATERIAL, &op.add.material)) {
			return NULL;
		}

		if (!mgcd_expect_var_resource(parser, MGCD_ENTRY_SHAPE, &op.add.shape)) {
			return NULL;
		}

		if (!mgcd_expect_var_v3i(parser, &op.add.translation)) {
			op.add.translation = mgcd_var_lit_v3i(V3i(0, 0, 0));
		}

		if (!mgcd_expect_var_int(parser, &op.add.rotation)) {
			op.add.rotation = mgcd_var_lit_int(0);
		}

	} else if (op_kind == atoms->area) {
		op.op = MGCD_AREA_AREA;

		if (!mgcd_expect_var_resource(parser, MGCD_ENTRY_AREA, &op.area.area)) {
			return NULL;
		}

		if (!mgcd_expect_var_v3i(parser, &op.add.translation)) {
			op.area.translation = mgcd_var_lit_v3i(V3i(0, 0, 0));
		}

		if (!mgcd_expect_var_int(parser, &op.add.rotation)) {
			op.area.rotation = mgcd_var_lit_int(0);
		}

	} else if (op_kind == atoms->remove) {
		op.op = MGCD_AREA_REMOVE;

		if (!mgcd_expect_var_resource(parser, MGCD_ENTRY_AREA, &op.remove.shape)) {
			return NULL;
		}

		if (!mgcd_expect_var_v3i(parser, &op.remove.translation)) {
			op.remove.translation = mgcd_var_lit_v3i(V3i(0, 0, 0));
		}

		if (!mgcd_expect_var_int(parser, &op.remove.rotation)) {
			op.remove.rotation = mgcd_var_lit_int(0);
		}

	} else {
		return NULL;
	}

	return mgcd_alloc_area_op(parser, op);
}

bool
mgcd_parse_area_block(struct mgcd_parser *parser,
		struct mgcd_area *area,
		struct mgcd_area_op **out_ops)
{
	struct mgcd_area_op *ops = NULL, **ops_ptr = &ops;

	while (mgcd_block_continue(mgcd_peek_token(parser))) {
		if (mgcd_peek_token(parser).type == MGCD_TOK_END_STMT) {
			mgcd_eat_token(parser);
			continue;
		}

		struct mgcd_area_op *stmt;
		stmt = mgcd_parse_area_op(parser);
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

struct mgcd_tmp_area_op {
	struct mgcd_tmp_area_op *next;
	struct mgc_area_op op;
};

struct mgcd_tmp_area {
	struct mgcd_tmp_area_op *ops;
	struct mgcd_tmp_area_op **head;
	size_t num_ops;

	bool error;
};

static struct mgcd_tmp_area_op *
mgcd_alloc_tmp_area_op(struct mgcd_context *ctx, struct mgc_area_op op)
{
	struct mgcd_tmp_area_op *result;
	result = arena_alloc(ctx->tmp_mem, sizeof(struct mgcd_tmp_area_op));
	result->next = NULL;
	result->op = op;
	return result;
}

static void
mgcd_append_tmp_area_op(struct mgcd_tmp_area *shape, struct mgcd_tmp_area_op *op)
{
	if (!shape->head) {
		shape->head = &shape->ops;
	}

	assert(op->next == NULL);
	assert(*shape->head == NULL);

	*shape->head = op;
	shape->head = &op->next;

	shape->num_ops += 1;
}

static void
mgcd_tmp_area_append_op(
		struct mgcd_context *ctx,
		struct mgcd_tmp_area *area,
		struct mgc_area_op op)
{
	mgcd_append_tmp_area_op(area, mgcd_alloc_tmp_area_op(ctx, op));
}

static void
mgcd_complete_area_internal(
		struct mgcd_context *ctx,
		struct mgcd_scope *scope,
		struct mgcd_area *area,
		struct mgcd_tmp_area *result,
		struct mgc_world_transform transform)
{
	struct mgcd_area_op *op;
	op = area->ops;

	while (op) {
		switch (op->op) {
			case MGCD_AREA_ADD:
				{
					struct mgc_area_op res = {0};
					res.op = MGC_AREA_ADD;

					v3i center = mgcd_var_value_v3i(scope, op->add.translation);
					int rotation = mgcd_var_value_int(scope, op->add.rotation);

					res.add.transform.translation = center;
					res.add.transform.xy_rotation = rotation;
					mgc_world_transform_multiply(&res.add.transform, transform);

					mgcd_resource_id shape_id, material_id;
					shape_id    = mgcd_var_value_mgcd_resource_id(scope, op->add.shape);
					material_id = mgcd_var_value_mgcd_resource_id(scope, op->add.material);

					res.add.shape = mgcd_expect_shape(ctx, shape_id);
					res.add.material = mgcd_expect_material(ctx, material_id);

					mgcd_tmp_area_append_op(ctx, result, res);
				}
				break;

			case MGCD_AREA_AREA:
				{
					mgc_error(ctx->err, op->loc,
							"TODO: Implement sub-areas.");
				}

			case MGCD_AREA_REMOVE:
				{
					struct mgc_area_op res = {0};
					res.op = MGC_AREA_REMOVE;

					v3i center = mgcd_var_value_v3i(scope, op->add.translation);
					int rotation = mgcd_var_value_int(scope, op->add.rotation);

					res.add.transform.translation = center;
					res.add.transform.xy_rotation = rotation;
					mgc_world_transform_multiply(&res.add.transform, transform);

					mgcd_resource_id shape_id;
					shape_id    = mgcd_var_value_mgcd_resource_id(scope, op->add.shape);

					res.add.shape = mgcd_expect_shape(ctx, shape_id);

					mgcd_tmp_area_append_op(ctx, result, res);
				}
				break;
		}

		op = op->next;
	}
}

int
mgcd_complete_area(struct mgcd_context *ctx,
		struct mgcd_area *area,
		struct arena *mem,
		struct mgc_area *out_area)
{
	arena_mark cp = arena_checkpoint(ctx->tmp_mem);

	struct mgcd_tmp_area result = {0};
	result.head = &result.ops;

	mgcd_complete_area_internal(
			ctx, NULL, area, &result,
			(struct mgc_world_transform){0});

	if (result.error) {
		arena_reset(ctx->tmp_mem, cp);
		return -1;
	}

	struct mgc_area_op *final_ops;
	final_ops = arena_allocn(mem, result.num_ops, sizeof(struct mgc_area_op));

	size_t ops_i = 0;
	struct mgcd_tmp_area_op *ops_iter = result.ops;

	while (ops_iter) {
		assert(ops_i < result.num_ops);

		final_ops[ops_i] = ops_iter->op;

		ops_i += 1;
		ops_iter = ops_iter->next;
	}
	assert(ops_i == result.num_ops);

	arena_reset(ctx->tmp_mem, cp);

	memset(out_area, 0, sizeof(struct mgc_area));
	out_area->ops = final_ops;
	out_area->num_ops = result.num_ops;

	return 0;
}
