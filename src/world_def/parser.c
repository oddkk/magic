#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "lexer.h"
#include "../utils.h"
#include "../config.h"

void
mgcd_parse_init(struct mgcd_parser *parser,
		struct mgcd_context *ctx,
		struct mgcd_lexer *lex)
{
	parser->ctx = ctx;
	parser->lex = lex;
}

struct mgcd_token
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

struct mgcd_token
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

bool
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

bool
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

bool
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

bool
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

bool
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

bool
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

bool
mgcd_expect_var_resource(struct mgcd_parser *parser, MGCD_TYPE(mgcd_resource_id) *out)
{
	struct mgcd_token tok;
	tok = mgcd_peek_token(parser);
	struct string path = {0};
	if (mgcd_try_get_path(tok, &path)) {
		mgcd_resource_id result;
		result = mgcd_request_resource_str(parser->ctx, parser->root_scope, path);

		if (result >= 0) {
			*out = mgcd_var_lit_mgcd_resource_id(result);
			return true;
		}
	}

	return false;
}

bool
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

bool
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

bool
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

bool
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

bool
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

bool
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
