#include <stdio.h>
#include <string.h>
#include "world_def.h"
#include "utils.h"
#include "config.h"

void
mgcd_parse_init(struct mgcd_parser *parser,
		struct mgcd_context *ctx,
		struct mgcd_lexer *lex)
{
	parser->ctx = ctx;
	parser->lex = lex;
}

static struct mgcd_token
mgcd_eat_token(struct mgcd_parser *parser)
{
	struct mgcd_token tok;
	if (parser->peek_buffer_count > 0) {
		tok = parser->peek_buffer[0];
		parser->peek_buffer_count = 0;
	} else {
		tok = mgcd_read_token(parser->lex);
#if MGCD_DEBUG_TOKENIZER
		mgcd_print_token(&tok);
#endif
	}

	return tok;
}

static struct mgcd_token
mgcd_peek_token(struct mgcd_parser *parser)
{
	if (parser->peek_buffer_count == 0) {
		parser->peek_buffer[0] = mgcd_read_token(parser->lex);
#if MGCD_DEBUG_TOKENIZER
		mgcd_print_token(&parser->peek_buffer[0]);
#endif
		parser->peek_buffer_count = 1;
	}

	struct mgcd_token tok;
	tok = parser->peek_buffer[0];
	return tok;
}

static bool
mgcd_try_get_ident(struct mgcd_token tok, struct atom **out_atom)
{
	if (tok.type != MGCD_TOK_IDENT) {
		return false;
	}

	if (out_atom) {
		*out_atom = tok.ident;
	}
	return true;
}


static bool
mgcd_try_eat_ident(struct mgcd_parser *parser, struct atom **out_atom)
{
	struct mgcd_token tok;
	tok = mgcd_peek_token(parser);
	if (mgcd_try_get_ident(tok, out_atom)) {
		mgcd_eat_token(parser);
		return true;
	}

	return false;
}

static bool
mgcd_try_get_version(struct mgcd_token tok, struct mgcd_version *out_version)
{
	if (tok.type != MGCD_TOK_VERSION) {
		return false;
	}

	if (out_version) {
		*out_version = tok.version;
	}
	return true;
}

static bool
mgcd_try_eat_version(struct mgcd_parser *parser, struct mgcd_version *out_version)
{
	struct mgcd_token tok;
	tok = mgcd_peek_token(parser);
	if (mgcd_try_get_version(tok, out_version)) {
		mgcd_eat_token(parser);
		return true;
	}

	return false;
}

static bool
mgcd_try_get_path(struct mgcd_token tok, struct string *out_path)
{
	if (tok.type != MGCD_TOK_FILE_PATH) {
		return false;
	}

	if (out_path) {
		*out_path = tok.file_path;
	}

	return true;
}

static bool
mgcd_expect_var_path(struct mgcd_parser *parser, struct string *out_path)
{
	struct mgcd_token tok;
	tok = mgcd_peek_token(parser);
	if (mgcd_try_get_path(tok, out_path)) {
		mgcd_eat_token(parser);
		return true;
	}

	return false;
}

static bool
mgcd_expect_var_int(struct mgcd_parser *parser, MGCD_TYPE(int) *out)
{
	struct mgcd_token tok;
	tok = mgcd_peek_token(parser);

	switch (tok.type) {

		case MGCD_TOK_IDENT:
			panic("TODO: Variables");
			return false;

		case MGCD_TOK_INTEGER_LIT:
			mgcd_eat_token(parser);
			*out = mgcd_var_lit_int(tok.integer_lit);
			return true;

		default:
			return false;
	}
}

static bool
mgcd_expect_tok(struct mgcd_parser *parser, enum mgcd_token_type type)
{
	struct mgcd_token tok;
	tok = mgcd_peek_token(parser);

	if (tok.type == type) {
		mgcd_eat_token(parser);
		return true;
	}
	return false;
}

#if 0
static bool
mgcd_try_get_int(struct mgcd_token tok, int *out_int)
{
	if (tok.type != MGCD_TOK_INTEGER_LIT) {
		return false;
	}

	if (out_int) {
		*out_int = tok.integer_lit;
	}

	return true;
}
#endif

static bool
mgcd_expect_int(struct mgcd_parser *parser, int *out)
{
	struct mgcd_token tok;
	tok = mgcd_peek_token(parser);

	switch (tok.type) {

		case MGCD_TOK_INTEGER_LIT:
			mgcd_eat_token(parser);
			*out = tok.integer_lit;
			return true;

		default:
			return false;
	}
}

static bool
mgcd_expect_var_v3i(struct mgcd_parser *parser, MGCD_TYPE(v3i) *out)
{
	struct mgcd_token tok;
	tok = mgcd_peek_token(parser);

	switch (tok.type) {

		case MGCD_TOK_IDENT:
			panic("TODO: Variables");
			return false;

		case MGCD_TOK_OPEN_TUPLE:
			{
				v3i result = {0};

				mgcd_eat_token(parser);

				for (size_t i = 0; i < 3; i++) {
					if (!mgcd_expect_int(parser, &result.m[i])) {
						return false;
					}
					if (!mgcd_expect_tok(parser, MGCD_TOK_ARG_SEP) && i != 2) {
						return false;
					}
				}

				if (!mgcd_expect_tok(parser, MGCD_TOK_CLOSE_TUPLE)) {
					return false;
				}

				*out = mgcd_var_lit_v3i(result);
				return true;
			}

		default:
			return false;
	}
}

static struct mgcd_shape_op *
mgcd_alloc_shape_op(struct mgcd_parser *parser, struct mgcd_shape_op op)
{
	struct paged_list *shape_ops = &parser->ctx->world->shape_ops;

	size_t new_op_id;
	new_op_id = paged_list_push(shape_ops);

	struct mgcd_shape_op *new_op;
	new_op = paged_list_get(shape_ops, new_op_id);

	*new_op = op;

	return new_op;
}

struct mgcd_shape_op *
mgcd_parse_shape_op(struct mgcd_parser *parser)
{
	struct atom *shape_kind;
	if (!mgcd_try_eat_ident(parser, &shape_kind)) {
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

		if (!mgcd_expect_var_path(parser, &op.heightmap.file)) {
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

static bool
mgcd_stmt_error_recover(struct mgcd_parser *parser)
{
	struct mgcd_token current_token;
	current_token = mgcd_peek_token(parser);
	while (current_token.type != MGCD_TOK_END_STMT &&
			current_token.type != MGCD_TOK_CLOSE_BLOCK &&
			current_token.type != MGCD_TOK_EOF) {
		mgcd_eat_token(parser);
		current_token = mgcd_peek_token(parser);
	}

	return current_token.type != MGCD_TOK_EOF;
}

static bool
mgcd_block_continue(struct mgcd_token tok)
{
	switch (tok.type) {
		case MGCD_TOK_EOF:
		case MGCD_TOK_CLOSE_BLOCK:
			return false;

		default:
			return true;
	}
}

bool
mgcd_parse_shape_block(struct mgcd_parser *parser, struct mgcd_shape_op **out_ops)
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
			mgc_error(parser->ctx->err, MGC_NO_LOC,
					"Expected ';'.");
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

	if (!mgcd_try_eat_version(&parser, &file->version)) {
		mgc_warning(parse_ctx->err, MGC_NO_LOC,
				"Missing version number. Assuming newest version.\n");
	}

	struct mgcd_shape_op *ops = NULL;
	bool result;
	result = mgcd_parse_shape_block(&parser, &ops);

	file->shape.ops = ops;

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
