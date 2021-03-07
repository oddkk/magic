#include "area.h"
#include "parser.h"
#include "../arena.h"

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
	if (!mgcd_try_eat_ident(parser, &op_kind)) {
		return NULL;
	}

	struct mgcd_atoms *atoms;
	atoms = &parser->ctx->atoms;

	struct mgcd_area_op op = {0};

	if (op_kind == atoms->add) {
		op.op = MGCD_AREA_ADD;

		if (!mgcd_expect_var_resource(parser, &op.add.material)) {
			return NULL;
		}

		if (!mgcd_expect_var_resource(parser, &op.add.shape)) {
			return NULL;
		}

		if (!mgcd_expect_var_v3i(parser, &op.add.translation)) {
			op.add.translation = mgcd_var_lit_v3i(V3i(0, 0, 0));
		}

		if (!mgcd_expect_var_int(parser, &op.add.rotation)) {
			op.add.rotation = mgcd_var_lit_int(0);
		}

	} else if (op_kind == atoms->remove) {
		op.op = MGCD_AREA_REMOVE;

		if (!mgcd_expect_var_resource(parser, &op.remove.shape)) {
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
