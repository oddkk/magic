#include "wd_material.h"
#include "wd_parser.h"

bool
mgcd_parse_material_op(
		struct mgcd_parser *parser,
		struct mgcd_material *mat)
{
	struct atom *op_kind;
	struct mgc_location op_loc = {0};
	if (!mgcd_try_eat_ident(parser, &op_kind, &op_loc)) {
		mgcd_report_unexpect_tok(parser, MGCD_TOK_IDENT);
		return false;
	}

	struct mgcd_atoms *atoms;
	atoms = &parser->ctx->atoms;

	if (op_kind == atoms->matref) {
		struct atom *name;
		struct mgc_location name_loc = {0};
		if (!mgcd_try_eat_ident(parser, &name, &name_loc)) {
			mgcd_report_unexpect_tok(parser, MGCD_TOK_IDENT);
			return false;
		}

		mat->name = name;
		mat->name_loc = name_loc;
	} else {
		mgc_error(parser->ctx->err, op_loc,
				"Operation '%.*s' is not valid for material.",
				ALIT(op_kind));
		return false;
	}

	return true;
}

bool
mgcd_parse_material_block(
		struct mgcd_parser *parser,
		struct mgcd_material *mat)
{
	while (mgcd_block_continue(mgcd_peek_token(parser))) {
		if (mgcd_peek_token(parser).type == MGCD_TOK_END_STMT) {
			mgcd_eat_token(parser);
			continue;
		}

		if (!mgcd_parse_material_op(parser, mat)) {
			continue;
		}

		if (!mgcd_expect_tok(parser, MGCD_TOK_END_STMT)) {
			mgcd_report_unexpect_tok(parser, MGCD_TOK_END_STMT);
			mgcd_stmt_error_recover(parser);
			continue;
		}
	}

	return true;
}
